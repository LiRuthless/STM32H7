"""SCB/1 monitor：常驻设备连接 + 实时记录 + 本地逐行 JSON 命令接口。

- 持有唯一设备 TCP（192.168.4.1:5000），订阅 UDP 实时数据并落盘。
- 本地接口只绑定 127.0.0.1:15000；端口被占用时明确报错退出，不抢占其他进程。
- 本地协议：逐行 JSON。请求 ``{"op":"...","args":{...},"id":N}``，
  响应一行 ``{"id":N,"status":"...","result":{...}}``（RESULT 样式 + 请求状态）。
- 终端状态每秒刷新一次，不逐条打印记录。
- Ctrl+C 有序退出：flush 并保留未完成文件，记录最后计数。
"""

from __future__ import annotations

import json
import socket
import threading
import time

import scb_client as client_mod
import scb_export as export_mod
import scb_output as output_mod
import scb_tune as tune_mod

LOCAL_HOST = "127.0.0.1"
LOCAL_PORT = 15000
STATUS_INTERVAL_S = 1.0

# 允许经本地接口转发的设备操作（不含远程起跑/停车/模式切换——协议本就没有）
_FORWARD_OPS = frozenset({
    "GET_STATUS", "GET_PARAMS", "SET_PARAMS", "TUNE_ENABLE", "TUNE_DISABLE",
    "GET_REQUEST_RESULT", "SUBSCRIBE", "GET_LOG_META",
})


class MonitorError(Exception):
    pass


class Monitor:
    def __init__(self, host: str = client_mod.DEFAULT_HOST,
                 port: int = client_mod.DEFAULT_TCP_PORT,
                 udp_port: int = client_mod.DEFAULT_UDP_PORT,
                 output_base: str = "run",
                 timeout: float = client_mod.DEFAULT_TIMEOUT_S,
                 local_port: int = LOCAL_PORT):
        self.host = host
        self.port = port
        self.udp_port = udp_port
        self.output_base = output_base
        self.timeout = timeout
        self.local_port = local_port
        self.recorder: output_mod.RunRecorder | None = None
        self.client: client_mod.DeviceClient | None = None
        self._stop = threading.Event()
        self._threads: list[threading.Thread] = []
        self._server: socket.socket | None = None
        self._start_time = time.time()

    # ------------------------------------------------------------------
    # 主流程
    # ------------------------------------------------------------------

    def run(self) -> int:
        """启动 monitor 并阻塞直到 Ctrl+C。返回进程退出码。"""
        self.recorder = output_mod.RunRecorder(self.output_base)
        self.recorder.start()
        self.client = client_mod.DeviceClient(
            host=self.host, port=self.port, udp_port=self.udp_port,
            timeout=self.timeout,
            event_handler=self._on_device_event,
            local_event_handler=self._on_local_event)
        try:
            ack = self.client.connect()
        except client_mod.ClientError as exc:
            print(f"[monitor] 连接设备失败：{exc}")
            self.recorder.close()
            return 2
        print(f"[monitor] 已连接 {self.host}:{self.port}，"
              f"session_id={self.client.session.session_id}，"
              f"link_ready={self.client.session.link_ready}")
        # 重连后先查询未决请求（绝不自动重放 SET_PARAMS）
        self.client.recover_pending()
        # 尝试取日志头做标定；没有有效头则 q 留空并记录事件
        try:
            meta = export_mod.get_log_meta(self.client)
            self.recorder.set_calibration(meta.header.black, meta.header.white)
            print(f"[monitor] 已取得标定（mode={meta.header.mode} "
                  f"gear={meta.header.gear}）")
        except client_mod.ClientError as exc:
            self.recorder.log_event({"event": "missing_log_header",
                                     "reason": str(exc)}, immediate=True)
            print(f"[monitor] 无有效日志头：{exc}（q 字段留空）")
        if self.client.session.link_ready:
            try:
                self.client.subscribe(self.udp_port)
                self.client.start_udp(self.udp_port)
            except client_mod.ClientError as exc:
                print(f"[monitor] SUBSCRIBE 失败：{exc}")
        self._start_local_server()
        self._spawn(self._telemetry_consumer, "scb-consumer")
        self._spawn(self._status_loop, "scb-status")
        try:
            while not self._stop.wait(0.5):
                pass
        except KeyboardInterrupt:
            print("\n[monitor] Ctrl+C，正在有序退出……")
        finally:
            self._shutdown()
        return 0

    def _shutdown(self) -> None:
        self._stop.set()
        if self._server is not None:
            try:
                self._server.close()
            except OSError:
                pass
        if self.client is not None:
            self.client.close()
        if self.recorder is not None:
            self.recorder.log_event({"event": "monitor_stop",
                                     "stats": dict(self.recorder.stats)},
                                    immediate=True)
            self.recorder.close()
        stats = self.recorder.stats if self.recorder else {}
        print(f"[monitor] 已退出。写入记录 {stats.get('records_written', 0)} 条，"
              f"重复 {stats.get('records_duplicate', 0)}，"
              f"缺口缺失 {stats.get('records_gap_missing', 0)}，"
              f"乱序 {stats.get('records_out_of_order', 0)}")

    def _spawn(self, target, name: str) -> None:
        thread = threading.Thread(target=target, name=name, daemon=True)
        thread.start()
        self._threads.append(thread)

    # ------------------------------------------------------------------
    # UDP 消费（独立线程；CSV 慢也不阻塞网络读线程）
    # ------------------------------------------------------------------

    def _telemetry_consumer(self) -> None:
        assert self.client is not None and self.recorder is not None
        while not self._stop.is_set():
            try:
                recv_time_ns, raw, packet = self.client.telemetry_queue.get(
                    timeout=0.5)
            except Exception:
                continue
            try:
                self.recorder.handle_packet(
                    recv_time_ns, raw, self.client.session.session_id, packet)
            except output_mod.RecordingError as exc:
                print(f"\n[monitor] 磁盘失败，记录会话停止：{exc}")
                self._stop.set()
                return

    # ------------------------------------------------------------------
    # 事件
    # ------------------------------------------------------------------

    def _on_device_event(self, event: dict) -> None:
        if self.recorder is not None:
            self.recorder.log_event(event, immediate=True)

    def _on_local_event(self, event: dict) -> None:
        if self.recorder is not None:
            self.recorder.log_event(event)

    # ------------------------------------------------------------------
    # 终端状态（每秒刷新一次）
    # ------------------------------------------------------------------

    def _status_loop(self) -> None:
        while not self._stop.wait(STATUS_INTERVAL_S):
            self._print_status()

    def _print_status(self) -> None:
        if self.client is None or self.recorder is None:
            return
        s = self.recorder.stats
        u = self.client.udp_stats
        print(f"\r[monitor] 运行 {int(time.time() - self._start_time)}s | "
              f"记录 {s['records_written']} 条 | 包 {s['packets_received']} | "
              f"重复 {s['records_duplicate']} | 缺口 {s['records_gap_missing']} | "
              f"乱序 {s['records_out_of_order']} | 坏包 {u['bad_packets']} | "
              f"溢出 {u['queue_overflow']} | 队列 "
              f"{self.client.telemetry_queue.qsize()}   ",
              end="", flush=True)

    # ------------------------------------------------------------------
    # 本地逐行 JSON 命令接口（只绑 loopback）
    # ------------------------------------------------------------------

    def _start_local_server(self) -> None:
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            server.bind((LOCAL_HOST, self.local_port))
        except OSError as exc:
            self._shutdown()
            raise MonitorError(
                f"本地端口 {LOCAL_HOST}:{self.local_port} 被占用：{exc}。"
                f"不抢占其他进程，请先停止已有 monitor") from exc
        server.listen(4)
        server.settimeout(0.5)
        self._server = server
        self._spawn(self._accept_loop, "scb-local-accept")
        print(f"\n[monitor] 本地命令接口：{LOCAL_HOST}:{self.local_port}（逐行 JSON）")

    def _accept_loop(self) -> None:
        while not self._stop.is_set():
            try:
                conn, _ = self._server.accept()
            except socket.timeout:
                continue
            except OSError:
                break
            self._spawn(lambda c=conn: self._serve_conn(c), "scb-local-conn")

    def _serve_conn(self, conn: socket.socket) -> None:
        with conn:
            conn.settimeout(None)
            buf = b""
            while not self._stop.is_set():
                try:
                    chunk = conn.recv(4096)
                except OSError:
                    break
                if not chunk:
                    break
                buf += chunk
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    line = line.strip()
                    if not line:
                        continue
                    response = self._handle_local_command(line)
                    try:
                        conn.sendall(response.encode("utf-8") + b"\n")
                    except OSError:
                        return

    def _handle_local_command(self, line: bytes) -> str:
        try:
            request = json.loads(line.decode("utf-8"))
        except (ValueError, UnicodeDecodeError) as exc:
            return json.dumps({"status": "INVALID_ARG",
                               "error": f"非法 JSON：{exc}"}, ensure_ascii=False)
        request_id = request.get("id")
        op = request.get("op")
        args = request.get("args") or {}
        response = {"id": request_id}
        try:
            if op == "STATUS":
                response["status"] = "OK"
                response["result"] = self._local_status()
            elif op == "SHUTDOWN":
                response["status"] = "OK"
                response["result"] = {"note": "monitor 即将退出"}
                self._stop.set()
            elif op in _FORWARD_OPS:
                assert self.client is not None
                result = self.client.request_final(op, args)
                response["status"] = result.get("status", "UNKNOWN")
                response["result"] = result
            else:
                response["status"] = "INVALID_ARG"
                response["error"] = f"不支持的操作 {op!r}"
        except client_mod.ClientError as exc:
            response["status"] = "IO_ERROR"
            response["error"] = str(exc)
        return json.dumps(response, ensure_ascii=False)

    def _local_status(self) -> dict:
        s = self.recorder.stats if self.recorder else {}
        return {
            "connected": self.client.connected if self.client else False,
            "session_id": self.client.session.session_id if self.client else 0,
            "link_ready": self.client.session.link_ready if self.client else False,
            "uptime_s": int(time.time() - self._start_time),
            "stats": s,
            "udp": self.client.udp_stats if self.client else {},
        }


def forward_to_monitor(op: str, args: dict,
                       local_port: int = LOCAL_PORT,
                       timeout: float = 10.0) -> dict:
    """把单条命令转交给本机 monitor（无 monitor 时抛 MonitorError）。"""
    try:
        with socket.create_connection((LOCAL_HOST, local_port),
                                      timeout=timeout) as conn:
            conn.sendall(json.dumps(
                {"op": op, "args": args, "id": 1},
                ensure_ascii=False).encode("utf-8") + b"\n")
            f = conn.makefile("r", encoding="utf-8")
            line = f.readline()
    except OSError as exc:
        raise MonitorError(
            f"无法连接本机 monitor（{LOCAL_HOST}:{local_port}）：{exc}。"
            f"没有 monitor 时请去掉 --monitor 直接连设备") from exc
    if not line:
        raise MonitorError("monitor 无响应")
    return json.loads(line)
