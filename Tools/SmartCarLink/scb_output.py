"""SCB/1 实时记录输出文件组。

输出文件（base 为 --output 指定名）：
- ``<base>.raw``：原始报文文件。每条：u64 本机接收时间 ns（小端）+ u32 报文长度
  （小端）+ 原始报文字节，按到达顺序追加。本机时间不代替设备采样 tick。
- ``<base>.csv``：57 列 CSV，首行列头；只写首次有效记录，重复不再次写；
  乱序记录仍写（属首次有效）并计入统计/事件，不插值补假样。
- ``<base>.metadata.json``：记录会话元信息（标定、文件段、起止时间等）。
- ``<base>.events.jsonl``：逐行 JSON 事件（设备事件 + 本地事件）。
- ``<base>.link_stats.json``：链路/记录统计，定期与退出时重写。

去重以 session_id + run_id + 记录索引联合判定；新 run（run_id 变化或索引归零）
新建文件段 ``<base>.seg<N>.raw/.csv``，不把新运行数据按索引误删。
批量写盘，至少每秒 flush；参数最终结果经 log_event(immediate=True) 立即 flush。
磁盘失败：停止该记录会话并抛 RecordingError 明确报告，不默默丢数据。
Ctrl+C：close() 有序收尾，保留未完成文件。
"""

from __future__ import annotations

import json
import os
import struct
import threading
import time

import scb_records as records

FLUSH_INTERVAL_S = 1.0
RAW_RECORD_FORMAT = "<QI"     # u64 本机接收时间 ns + u32 报文长度
RAW_RECORD_HEADER_SIZE = struct.calcsize(RAW_RECORD_FORMAT)


class RecordingError(OSError):
    """记录会话磁盘失败。"""


class _Segment:
    """一个运行段（同一 session_id + run_id）的文件与去重状态。"""

    def __init__(self, index: int, session_id: int, run_id: int,
                 raw_file, csv_file, csv_needs_header: bool):
        self.index = index
        self.session_id = session_id
        self.run_id = run_id
        self.raw_file = raw_file
        self.csv_file = csv_file
        self.csv_needs_header = csv_needs_header
        self.seen_indices: set[int] = set()      # 已写入的记录索引
        self.expected_next: int | None = None    # 期望的下一个索引
        self.last_tick: int | None = None        # 上一条记录的 tick
        self.records_written = 0


class RunRecorder:
    """实时记录器。handle_packet 通常在 UDP 消费线程调用；log_event 线程安全。"""

    def __init__(self, base_path: str, flush_interval: float = FLUSH_INTERVAL_S):
        self.base_path = base_path
        self.flush_interval = flush_interval
        self.failed: str | None = None          # 磁盘失败原因；非 None 即已停止
        # RLock：handle_packet 持锁时会经 _handle_record_locked 内部调用 log_event
        self._lock = threading.RLock()
        self._segment: _Segment | None = None
        self._segments_info: list[dict] = []
        self._calibration: tuple[tuple[int, ...], tuple[int, ...]] | None = None
        self._calibration_noted = False
        self._start_time_ns = time.time_ns()
        self._stop_event = threading.Event()
        self._flush_thread: threading.Thread | None = None
        self._events_file = None
        self.stats = {
            "packets_received": 0,
            "records_received": 0,
            "records_written": 0,
            "records_duplicate": 0,
            "records_out_of_order": 0,
            "records_gap_missing": 0,     # 缺口中缺失的记录数
            "gap_count": 0,               # 缺口次数
            "run_count": 0,
            "tick_wraps": 0,              # tick 回绕次数
        }

    # ------------------------------------------------------------------
    # 生命周期
    # ------------------------------------------------------------------

    def start(self) -> None:
        """打开事件文件并启动每秒 flush 线程。"""
        os.makedirs(os.path.dirname(os.path.abspath(self.base_path)), exist_ok=True)
        try:
            self._events_file = open(
                self.base_path + ".events.jsonl", "a", encoding="utf-8")
        except OSError as exc:
            self._fail(f"打开事件文件失败：{exc}")
        self._flush_thread = threading.Thread(
            target=self._flush_loop, name="scb-flush", daemon=True)
        self._flush_thread.start()

    def close(self) -> None:
        """有序收尾：flush 并关闭全部文件，保留未完成文件，写最终统计。"""
        self._stop_event.set()
        if self._flush_thread is not None:
            self._flush_thread.join(timeout=2.0)
            self._flush_thread = None
        with self._lock:
            try:
                self._write_metadata()
                self._write_stats()
            except OSError:
                pass
            self._close_segment_locked()
            if self._events_file is not None:
                try:
                    self._events_file.flush()
                    self._events_file.close()
                except OSError:
                    pass
                self._events_file = None

    def __enter__(self) -> "RunRecorder":
        self.start()
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    def _fail(self, message: str) -> None:
        self.failed = message
        raise RecordingError(message)

    # ------------------------------------------------------------------
    # 标定（来自 GET_LOG_META 的日志头）
    # ------------------------------------------------------------------

    def set_calibration(self, black: tuple[int, ...], white: tuple[int, ...]) -> None:
        with self._lock:
            self._calibration = (tuple(black), tuple(white))
            self._calibration_noted = False

    # ------------------------------------------------------------------
    # 事件
    # ------------------------------------------------------------------

    def log_event(self, event: dict, immediate: bool = False) -> None:
        """写一行事件 JSON；immediate=True（如参数最终结果）立即 flush。"""
        line = json.dumps(event, ensure_ascii=False, default=str)
        with self._lock:
            if self._events_file is None:
                return
            try:
                self._events_file.write(line + "\n")
                if immediate:
                    self._events_file.flush()
            except OSError as exc:
                self._fail(f"写事件文件失败：{exc}")

    # ------------------------------------------------------------------
    # 遥测包处理
    # ------------------------------------------------------------------

    def handle_packet(self, recv_time_ns: int, raw_frame: bytes,
                      session_id: int, packet: records.TelemetryPacket) -> None:
        """处理一个已验证的 TELEMETRY 包（写 raw + CSV + 统计）。"""
        with self._lock:
            if self.failed is not None:
                raise RecordingError(self.failed)
            try:
                self._handle_packet_locked(recv_time_ns, raw_frame, session_id, packet)
            except OSError as exc:
                self._fail(f"写记录文件失败：{exc}")

    def _handle_packet_locked(self, recv_time_ns, raw_frame, session_id, packet):
        self.stats["packets_received"] += 1
        segment = self._segment
        if (segment is None or segment.run_id != packet.run_id
                or segment.session_id != session_id):
            # 新运行/新会话：新建文件段。延迟抵达的上一运行包归入旧段？——
            # 旧段文件已关闭则视为乱序包统计，不拼入新段。
            segment = self._find_or_open_segment(session_id, packet.run_id)
        # 原始报文按到达顺序保存
        segment.raw_file.write(
            struct.pack(RAW_RECORD_FORMAT, recv_time_ns, len(raw_frame)))
        segment.raw_file.write(raw_frame)
        for i, record in enumerate(packet.records):
            index = packet.first_index + i
            self._handle_record_locked(segment, index, record)

    def _find_or_open_segment(self, session_id: int, run_id: int) -> _Segment:
        # 运行/会话边界即新建文件段；迟到旧运行包也单独开段保存原始数据并标记，
        # 不按索引归并进当前运行。
        self._close_segment_locked()
        index = len(self._segments_info) + 1
        if index == 1:
            raw_path = self.base_path + ".raw"
            csv_path = self.base_path + ".csv"
        else:
            raw_path = f"{self.base_path}.seg{index}.raw"
            csv_path = f"{self.base_path}.seg{index}.csv"
        raw_file = open(raw_path, "ab")
        csv_exists = os.path.exists(csv_path) and os.path.getsize(csv_path) > 0
        csv_file = open(csv_path, "a", encoding="utf-8", newline="")
        segment = _Segment(index, session_id, run_id, raw_file, csv_file,
                           csv_needs_header=not csv_exists)
        self._segment = segment
        self.stats["run_count"] += 1
        self._segments_info.append({
            "index": index, "session_id": session_id, "run_id": run_id,
            "raw_file": raw_path, "csv_file": csv_path,
        })
        return segment

    def _handle_record_locked(self, segment: _Segment, index: int,
                              record: records.LogRecord) -> None:
        self.stats["records_received"] += 1
        # 联合去重：session_id + run_id（段）+ 记录索引
        if index in segment.seen_indices:
            self.stats["records_duplicate"] += 1
            return  # 重复不再次写
        if segment.expected_next is not None:
            if index > segment.expected_next:
                gap = index - segment.expected_next
                self.stats["records_gap_missing"] += gap
                self.stats["gap_count"] += 1
                self.log_event({
                    "event": "record_gap", "run_id": segment.run_id,
                    "from_index": segment.expected_next, "to_index": index - 1,
                    "missing": gap,
                })
            elif index < segment.expected_next:
                # 晚到的未见过记录：乱序，明确标记，不插值
                self.stats["records_out_of_order"] += 1
                self.log_event({
                    "event": "record_out_of_order", "run_id": segment.run_id,
                    "index": index, "expected_next": segment.expected_next,
                })
        # tick 回绕：同一运行内 tick 小于上一条（u32 自然回绕）
        if segment.last_tick is not None and record.tick < segment.last_tick:
            self.stats["tick_wraps"] += 1
            self.log_event({
                "event": "tick_wrap", "run_id": segment.run_id,
                "index": index, "prev_tick": segment.last_tick,
                "tick": record.tick,
            })
        segment.seen_indices.add(index)
        segment.last_tick = record.tick
        if segment.expected_next is None or index >= segment.expected_next:
            segment.expected_next = index + 1
        # CSV：首次有效记录才写
        if segment.csv_needs_header:
            segment.csv_file.write(records.csv_header_line() + "\n")
            segment.csv_needs_header = False
        if self._calibration is not None:
            black, white = self._calibration
        else:
            black = white = None
            if not self._calibration_noted:
                self._calibration_noted = True
                self.log_event({"event": "missing_log_header",
                                "note": "没有有效日志头，q 字段留空，不猜标定"})
        row = records.record_to_csv_row(record, black, white)
        segment.csv_file.write(",".join(row) + "\n")
        segment.records_written += 1
        self.stats["records_written"] += 1

    def _close_segment_locked(self) -> None:
        segment = self._segment
        self._segment = None
        if segment is None:
            return
        for f in (segment.raw_file, segment.csv_file):
            if f is not None:
                try:
                    f.flush()
                    f.close()
                except OSError:
                    pass
        for info in self._segments_info:
            if info["index"] == segment.index:
                info["records_written"] = segment.records_written

    # ------------------------------------------------------------------
    # 定期 flush 与统计
    # ------------------------------------------------------------------

    def flush(self) -> None:
        """立即 flush 当前所有文件并重写统计。"""
        with self._lock:
            self._flush_locked()

    def _flush_locked(self) -> None:
        if self._segment is not None:
            for f in (self._segment.raw_file, self._segment.csv_file):
                if f is not None:
                    f.flush()
        if self._events_file is not None:
            self._events_file.flush()
        self._write_stats()

    def _flush_loop(self) -> None:
        while not self._stop_event.wait(self.flush_interval):
            with self._lock:
                if self.failed is not None:
                    return
                try:
                    self._flush_locked()
                except OSError as exc:
                    self.failed = f"定期 flush 失败：{exc}"
                    return

    def _write_stats(self) -> None:
        stats = dict(self.stats)
        stats["failed"] = self.failed
        with open(self.base_path + ".link_stats.json", "w", encoding="utf-8") as f:
            json.dump(stats, f, ensure_ascii=False, indent=2)

    def _write_metadata(self) -> None:
        meta = {
            "base": os.path.basename(self.base_path),
            "start_time_ns": self._start_time_ns,
            "end_time_ns": time.time_ns(),
            "calibration": (
                {"black": list(self._calibration[0]),
                 "white": list(self._calibration[1])}
                if self._calibration is not None else None),
            "segments": self._segments_info,
            "note": "实时上传可丢；参数历史见 events.jsonl，"
                    "单独实时文件不能完整还原运行调参",
        }
        with open(self.base_path + ".metadata.json", "w", encoding="utf-8") as f:
            json.dump(meta, f, ensure_ascii=False, indent=2)
