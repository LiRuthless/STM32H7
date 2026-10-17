"""模拟 ESP32（SCB/1 对端），仅用于单元测试。

能力：
- HELLO/HELLO_ACK 握手（可配 session_id、link_ready）。
- COMMAND 处理：默认内置 GET_STATUS/GET_PARAMS/SET_PARAMS/SUBSCRIBE/
  GET_REQUEST_RESULT 及日志导出四操作；可用 command_handler 完全接管。
- 结果脚本：set_result_script(op, [body, ...], delay) 先回 RECEIVED 再延迟回
  APPLIED（业务结果延迟）。
- 发送分片 chunk_size：任意拆包粘包。
- withhold_acks：吞掉确认，观察客户端 200ms×3 重发与链路死亡。
- 可靠去重：同 seq 客户端重发 → 重发缓存 RESULT，不重复执行。
- 会话变化：new_session() 后旧会话帧丢弃；重连握手返回新 session_id。
- 日志导出：set_log_image() 提供内存日志；log_read_hook 可注入重复块/错偏移；
  end_crc32_override 可制造整体 CRC 失败。
- UDP：send_telemetry() 从本端发送遥测数据报。
"""

from __future__ import annotations

import socket
import struct
import threading
import time
import zlib

import scb_protocol as protocol
import scb_records as records

DEFAULT_VALUES = {"kp": 2.0, "k2p": 0.008, "kd": 15.0, "kp_v": 20.0,
                  "ki_v": 0.75, "base_speed": 180, "fan_run": 1600,
                  "fan_idle": 1100}
DEFAULT_LIMITS = {k: {"min": 0, "max": 100} for k in
                  ("kp", "k2p", "kd", "kp_v", "ki_v")}
DEFAULT_LIMITS.update({"base_speed": {"min": 0, "max": 300},
                       "fan_run": {"min": 0, "max": 10000},
                       "fan_idle": {"min": 0, "max": 10000}})


class MockESP32:
    def __init__(self, session_id: int = 0x1234, link_ready: bool = True):
        self.session_id = session_id
        self.link_ready = link_ready
        self.port = 0
        self.running = False

        # 行为钩子
        self.chunk_size: int | None = None       # 发送分片大小（拆包粘包）
        self.withhold_acks = False               # 吞确认，观察客户端重发
        self.result_scripts: dict[str, list[dict]] = {}
        self.result_script_delay = 0.3
        self.command_handler = None              # fn(op, args) -> dict | None
        self.status_body: dict = {}              # GET_STATUS 附加字段

        # 观察记录
        self.received_commands: list[dict] = []  # {"request_id","op","args","seq"}
        self.received_seqs: list[int] = []       # 全部收到的可靠帧 seq
        self.executed: list[dict] = []           # 实际执行（去重后）的命令

        # 请求结果缓存（模拟 STM32 最近 32 个请求）
        self.results_cache: dict[int, tuple[bytes, dict]] = {}

        # 日志导出
        self._log_image: bytes | None = None
        self._log_header: records.LogHeader | None = None
        self.allow_begin = True
        self.log_read_hook = None    # fn(offset, data) -> (offset, data) 或 None(不改)
        self.log_read_duplicate = False  # 每块重复发送一次
        self.end_crc32_override: int | None = None

        self._tx_seq = 0
        self._rx_reliable_seqs: set[int] = set()
        self._ack_seq = 0
        self._transfer_id = 0

        self._server: socket.socket | None = None
        self._conn: socket.socket | None = None
        self._parser = protocol.FrameParser()
        self._lock = threading.Lock()
        self._accept_thread: threading.Thread | None = None
        self._udp_sock: socket.socket | None = None

    # ------------------------------------------------------------------
    # 生命周期
    # ------------------------------------------------------------------

    def start(self) -> int:
        self._server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._server.bind(("127.0.0.1", 0))
        self._server.listen(1)
        self._server.settimeout(0.2)
        self.port = self._server.getsockname()[1]
        self.running = True
        self._accept_thread = threading.Thread(target=self._accept_loop,
                                               daemon=True)
        self._accept_thread.start()
        return self.port

    def stop(self) -> None:
        self.running = False
        for s in (self._conn, self._server, self._udp_sock):
            if s is not None:
                try:
                    s.close()
                except OSError:
                    pass
        if self._accept_thread is not None:
            self._accept_thread.join(timeout=2.0)

    def new_session(self, session_id: int) -> None:
        """会话变化：旧会话帧丢弃，重连握手返回新 session_id。"""
        with self._lock:
            self.session_id = session_id
            self._rx_reliable_seqs.clear()
            self._ack_seq = 0
            self.results_cache.clear()

    def drop_connection(self) -> None:
        """主动断开当前 TCP（SPI 会话保留）。"""
        conn, self._conn = self._conn, None
        if conn is not None:
            try:
                conn.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            try:
                conn.close()
            except OSError:
                pass
        self._parser = protocol.FrameParser()

    # ------------------------------------------------------------------
    # 连接与收帧
    # ------------------------------------------------------------------

    def _accept_loop(self) -> None:
        while self.running:
            try:
                conn, _ = self._server.accept()
            except socket.timeout:
                continue
            except OSError:
                break
            if self._conn is not None:
                # 第二个控制连接返回 BUSY 后关闭
                try:
                    conn.sendall(b"BUSY")
                    conn.close()
                except OSError:
                    pass
                continue
            self._conn = conn
            # 每条新 TCP 连接独立维护 seq/ack 与可靠去重状态
            self._parser = protocol.FrameParser()
            with self._lock:
                self._rx_reliable_seqs.clear()
                self._ack_seq = 0
            threading.Thread(target=self._read_loop, args=(conn,),
                             daemon=True).start()

    def _read_loop(self, conn: socket.socket) -> None:
        while self.running and self._conn is conn:
            try:
                chunk = conn.recv(4096)
            except OSError:
                break
            if not chunk:
                break
            try:
                frames = self._parser.feed(chunk)
            except protocol.ProtocolError:
                break
            for header, payload in frames:
                self._handle_frame(header, payload)

    def _handle_frame(self, header: protocol.FrameHeader, payload: bytes) -> None:
        if header.msg_type == protocol.MSG_HELLO:
            self._send(protocol.MSG_HELLO_ACK, protocol.encode_json_payload({
                "protocol": 1, "session_id": self.session_id,
                "link_ready": self.link_ready,
            }), reliable=False, seq=0, session_id=0)
            return
        if header.session_id != self.session_id:
            return  # 旧会话帧丢弃
        if header.reliable:
            self.received_seqs.append(header.seq)
            if header.seq in self._rx_reliable_seqs:
                # 客户端可靠重发：重发缓存 RESULT，不重复执行
                cached = self.results_cache.get(header.request_id)
                if cached is not None:
                    _, body = cached
                    self._send_result(header.request_id, body)
                return
            self._rx_reliable_seqs.add(header.seq)
            self._ack_seq = header.seq
        if header.msg_type == protocol.MSG_COMMAND:
            try:
                body = protocol.decode_json_payload(payload)
            except protocol.ProtocolError:
                return
            self._handle_command(header, payload, body)

    # ------------------------------------------------------------------
    # 命令处理
    # ------------------------------------------------------------------

    def _handle_command(self, header, payload: bytes, body: dict) -> None:
        op = body.get("op", "")
        args = body.get("args", {})
        request_id = header.request_id
        self.received_commands.append({"request_id": request_id, "op": op,
                                       "args": args, "seq": header.seq})
        # 结果脚本：分阶段发送（如先 RECEIVED 后 APPLIED）
        if op in self.result_scripts:
            stages = self.result_scripts[op]
            self._execute_and_cache(request_id, payload, stages[0], op)
            for stage in stages[1:]:
                threading.Timer(self.result_script_delay, self._send_staged,
                                args=(request_id, payload, stage, op)).start()
            return
        response = self._dispatch_command(op, args, request_id, payload)

    def _send_staged(self, request_id, payload, stage, op) -> None:
        self._execute_and_cache(request_id, payload, stage, op)

    def _execute_and_cache(self, request_id, payload, body, op) -> None:
        body = dict(body)
        body.setdefault("op", op)
        self.executed.append({"request_id": request_id, "op": op, "body": body})
        if body.get("status") != "RECEIVED":
            self.results_cache[request_id] = (payload, body)
            while len(self.results_cache) > 32:
                self.results_cache.pop(next(iter(self.results_cache)))
        self._send_result(request_id, body)

    def _dispatch_command(self, op, args, request_id, payload) -> None:
        if self.command_handler is not None:
            response = self.command_handler(op, args)
            if response is not None:
                self._execute_and_cache(request_id, payload, response, op)
                return
            # 返回 None：回退到内置默认处理
        handler = getattr(self, f"_op_{op}", None)
        if handler is None:
            self._execute_and_cache(request_id, payload,
                                    {"status": "NOT_FOUND"}, op)
            return
        handler(args, request_id, payload, op)

    # -- 内置操作 --

    def _op_GET_STATUS(self, args, request_id, payload, op):
        body = {"status": "OK", "running": False, "mode": 1, "gear": 0,
                "element": "normal", "revision": 3, "tick": 123456,
                "control_sequence": 6000, "log_valid": self._log_image is not None,
                "run_id": 7}
        body.update(self.status_body)
        self._execute_and_cache(request_id, payload, body, op)

    def _op_GET_PARAMS(self, args, request_id, payload, op):
        self._execute_and_cache(request_id, payload, {
            "status": "OK", "values": dict(DEFAULT_VALUES),
            "limits": DEFAULT_LIMITS, "effective": dict(DEFAULT_VALUES),
            "overridden": False}, op)

    def _op_SET_PARAMS(self, args, request_id, payload, op):
        values = args.get("values", {})
        self._execute_and_cache(request_id, payload, {
            "status": "APPLIED", "values": values, "revision": 4,
            "tick": 123500, "control_sequence": 6100}, op)

    def _op_TUNE_ENABLE(self, args, request_id, payload, op):
        self._execute_and_cache(request_id, payload, {
            "status": "OK", "tick": 123600, "control_sequence": 6150}, op)

    def _op_TUNE_DISABLE(self, args, request_id, payload, op):
        self._execute_and_cache(request_id, payload, {"status": "OK"}, op)

    def _op_SUBSCRIBE(self, args, request_id, payload, op):
        port = args.get("udp_port")
        if not isinstance(port, int) or not (1 <= port <= 65535):
            self._execute_and_cache(request_id, payload,
                                    {"status": "INVALID_ARG"}, op)
            return
        self._execute_and_cache(request_id, payload, {"status": "OK"}, op)

    def _op_GET_REQUEST_RESULT(self, args, request_id, payload, op):
        target = args.get("target_request_id")
        cached = self.results_cache.get(target)
        if cached is None:
            self._execute_and_cache(request_id, payload,
                                    {"status": "NOT_FOUND"}, op)
        else:
            _, cached_body = cached
            self._execute_and_cache(request_id, payload, {
                "status": "OK", "target_request_id": target,
                "result": cached_body}, op)

    # -- 日志导出 --

    def set_log_image(self, header: records.LogHeader,
                      recs: list[records.LogRecord]) -> None:
        self._log_header = header
        self._log_image = records.pack_log_header(header) + b"".join(
            records.pack_record(r) for r in recs)

    def _op_GET_LOG_META(self, args, request_id, payload, op):
        if self._log_image is None:
            self._execute_and_cache(request_id, payload, {
                "status": "OK", "log_valid": False, "header_hex": "",
                "written_records": 0}, op)
            return
        header_bytes = self._log_image[:records.LOG_HEADER_SIZE]
        self._execute_and_cache(request_id, payload, {
            "status": "OK", "log_valid": True,
            "header_hex": header_bytes.hex(),
            "written_records": (len(self._log_image) - records.LOG_HEADER_SIZE)
                               // records.RECORD_SIZE}, op)

    def _op_LOG_BEGIN(self, args, request_id, payload, op):
        if self._log_image is None:
            self._execute_and_cache(request_id, payload,
                                    {"status": "NOT_FOUND"}, op)
            return
        if not self.allow_begin:
            self._execute_and_cache(request_id, payload,
                                    {"status": "BUSY"}, op)
            return
        self._transfer_id += 1
        total = len(self._log_image)
        self._execute_and_cache(request_id, payload, {
            "status": "OK", "transfer_id": self._transfer_id,
            "total_bytes": total,
            "record_count": (total - records.LOG_HEADER_SIZE) // records.RECORD_SIZE,
            "header_size": records.LOG_HEADER_SIZE,
            "record_size": records.RECORD_SIZE}, op)

    def _op_LOG_READ(self, args, request_id, payload, op):
        if self._log_image is None:
            self._send_result(request_id, {"op": op, "status": "NOT_FOUND"})
            return
        offset = args.get("offset", 0)
        length = args.get("length", 0)
        if not (1 <= length <= 960) or offset + length > len(self._log_image):
            self._send_result(request_id, {"op": op, "status": "INVALID_ARG"})
            return
        data = self._log_image[offset:offset + length]
        if self.log_read_hook is not None:
            modified = self.log_read_hook(offset, data)
            if modified is not None:
                offset, data = modified
        block = records.pack_log_data(offset, data)
        self._send(protocol.MSG_LOG_DATA, block, request_id=request_id)
        if self.log_read_duplicate:
            self._send(protocol.MSG_LOG_DATA, block, request_id=request_id)

    def _op_LOG_END(self, args, request_id, payload, op):
        if self._log_image is None:
            self._execute_and_cache(request_id, payload,
                                    {"status": "NOT_FOUND"}, op)
            return
        crc = self.end_crc32_override
        if crc is None:
            crc = zlib.crc32(self._log_image) & 0xFFFFFFFF
        self._execute_and_cache(request_id, payload, {
            "status": "OK", "total_bytes": len(self._log_image),
            "crc32": crc}, op)

    # ------------------------------------------------------------------
    # 发送（支持分片制造拆包粘包）
    # ------------------------------------------------------------------

    def _send_result(self, request_id: int, body: dict) -> None:
        if self.withhold_acks:
            return  # 吞确认：客户端应 200ms 重发
        self._send(protocol.MSG_RESULT, protocol.encode_json_payload(body),
                   request_id=request_id)

    def send_event(self, body: dict) -> None:
        """主动发 EVENT（request_id=0）。"""
        self._send(protocol.MSG_EVENT, protocol.encode_json_payload(body),
                   request_id=0)

    def _send(self, msg_type: int, payload: bytes, request_id: int = 0,
              reliable: bool | None = None, seq: int | None = None,
              session_id: int | None = None) -> None:
        with self._lock:
            conn = self._conn
            if conn is None:
                return
            if seq is None:
                self._tx_seq += 1
                seq = self._tx_seq
            frame = protocol.build_frame(
                msg_type, payload,
                session_id=self.session_id if session_id is None else session_id,
                seq=seq, ack_seq=self._ack_seq, request_id=request_id,
                flags=(protocol.default_flags(msg_type) if reliable is None
                       else (protocol.FLAG_RELIABLE if reliable else 0)))
            try:
                if self.chunk_size:
                    view = memoryview(frame)
                    while view:
                        piece = view[:self.chunk_size]
                        conn.sendall(piece)
                        view = view[len(piece):]
                        time.sleep(0.001)
                else:
                    conn.sendall(frame)
            except OSError:
                pass

    # ------------------------------------------------------------------
    # UDP 遥测发送
    # ------------------------------------------------------------------

    def send_telemetry(self, udp_port: int, packet_records, first_index: int,
                       run_id: int = 7, revision: int = 3,
                       session_id: int | None = None,
                       first_tick: int | None = None) -> bytes:
        """构造并发送一帧 TELEMETRY UDP 数据报，返回帧字节。"""
        payload = records.pack_telemetry(
            first_index, revision, 0, 0,
            packet_records[0].tick if first_tick is None else first_tick,
            run_id, packet_records)
        with self._lock:
            self._tx_seq += 1
            seq = self._tx_seq
        frame = protocol.build_frame(
            protocol.MSG_TELEMETRY, payload,
            session_id=self.session_id if session_id is None else session_id,
            seq=seq, record_count=len(packet_records))
        sock = self._udp_sock
        if sock is None:
            sock = self._udp_sock = socket.socket(socket.AF_INET,
                                                  socket.SOCK_DGRAM)
        sock.sendto(frame, ("127.0.0.1", udp_port))
        return frame


def make_record(tick: int, i: int = 0) -> records.LogRecord:
    """测试用日志记录。"""
    return records.LogRecord(
        tick=tick, adc=(100 + i, 200, 300, 400), speed_l=10, speed_r=11,
        track_error=-5, track_out=6, gyro_x=1, gyro_y=2, gyro_z=3,
        dl1b_mm=records.DL1B_INVALID_MM, battery=740, kernel_state=2,
        roundabout_state=0, gray_raw=tuple(100 + 10 * j for j in range(16)),
        gray_sequence=1000 + i, control_sequence=2000 + i, em_error=-10,
        optical_error=20, protection_flags=0, mode=1, optical_state=1,
        gray_valid=1, stop_reason=0)


def make_header(mode: int = 1, gear: int = 0) -> records.LogHeader:
    """有效标定的测试日志头。"""
    return records.LogHeader(
        mode=mode, gear=gear,
        black=tuple([100] * 16), white=tuple([3000] * 16),
        em_kp=2.0, em_k2p=0.008, em_kd=15.0, em_base_speed=180,
        opt_kp=1.5, opt_k2p=0.004, opt_kd=12.0, opt_base_speed=150,
        kp_v=20.0, ki_v=0.75, fan_run=1600, fan_idle=1100,
        gray_budget_us=800, control_budget_us=1500, run_delay_count=100)
