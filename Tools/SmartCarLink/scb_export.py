"""SCB/1 停车日志完整导出与离线转换。

导出流程（只在停车且日志收尾完成后由设备许可 LOG_BEGIN）：
1. GET_LOG_META 读缓存头：log_valid、header_hex（256B 头的 512 位十六进制）、
   written_records；校验头长度/magic/version/CRC16/标定。
2. LOG_BEGIN → transfer_id、total_bytes、record_count、header_size=256、record_size=96；
   校验 total_bytes == 256 + 96*record_count。
3. 窗口 1 顺序 LOG_READ：length 为 1~960，末块按剩余字节；验证 request_id、
   offset、length、边界；可靠重发的重复块不重写、不重复累计整体 CRC。
4. 流式写 ``<output>.partial`` 并累计 CRC32。
5. LOG_END 返回 total_bytes/crc32，二者均一致且头校验通过才改名为
   ``<output>.bin`` 并生成 57 列 CSV（含 #log_version/#black/#white 前导）。
6. 任何失败保留 .partial 及原因（``.partial.reason.txt``）；重连新建导出从 0
   开始，不自动续传旧会话。

离线 convert：完整二进制日志 + 可选 events.jsonl → 57 列 CSV；
没有事件文件时明确注明参数历史不完整。
"""

from __future__ import annotations

import binascii
import json
import os
import zlib
from dataclasses import dataclass

import scb_client as client_mod
import scb_records as records

LOG_READ_MAX_LENGTH = 960


@dataclass
class ExportResult:
    """导出结果。ok=False 时 reason 说明失败原因，.partial 保留。"""

    ok: bool
    reason: str = ""
    final_path: str | None = None
    partial_path: str | None = None
    csv_path: str | None = None
    total_bytes: int = 0
    record_count: int = 0
    crc32: int = 0


@dataclass
class LogMeta:
    """GET_LOG_META 校验后的结果。"""

    header: records.LogHeader
    header_bytes: bytes
    written_records: int


class ExportError(client_mod.ClientError):
    """导出失败。"""


def get_log_meta(client: client_mod.DeviceClient) -> LogMeta:
    """GET_LOG_META 读缓存头并校验（不读写 Flash）。"""
    result = client.request_final("GET_LOG_META", {})
    status = str(result.get("status", ""))
    if status != "OK":
        raise RequestRejectedFromMeta(status, result)
    if not result.get("log_valid", False):
        raise ExportError("设备无有效日志（log_valid=false），可能未运行或未停车收尾")
    header_hex = result.get("header_hex", "")
    if not isinstance(header_hex, str) or len(header_hex) != 512:
        raise ExportError(f"header_hex 应为 512 位十六进制，实际 {len(header_hex)}")
    try:
        header_bytes = binascii.unhexlify(header_hex)
    except (binascii.Error, ValueError) as exc:
        raise ExportError(f"header_hex 非法：{exc}") from exc
    header = records.validate_log_header(header_bytes)  # 长度/magic/version/CRC/标定
    written = result.get("written_records", 0)
    if not isinstance(written, int) or written < 0:
        raise ExportError(f"written_records 非法：{written!r}")
    return LogMeta(header=header, header_bytes=header_bytes, written_records=written)


class RequestRejectedFromMeta(ExportError):
    def __init__(self, status: str, result: dict):
        super().__init__(f"GET_LOG_META 返回 {status}")
        self.status = status
        self.result = result


class LogExporter:
    """完整导出器。重连后必须新建实例或重新 export（从 0 开始，不续传）。"""

    def __init__(self, client: client_mod.DeviceClient, event_logger=None):
        self.client = client
        self._event_logger = event_logger

    def _log(self, event: dict) -> None:
        if self._event_logger is not None:
            self._event_logger(event)

    def export(self, output_base: str, on_progress=None) -> ExportResult:
        """执行完整导出。output_base 为不带扩展名的输出名。"""
        partial_path = output_base + ".partial"
        final_path = output_base + ".bin"
        csv_path = output_base + ".csv"
        crc = 0
        written = 0
        try:
            meta = get_log_meta(self.client)
            begin = self.client.request_final("LOG_BEGIN", {})
            status = str(begin.get("status", ""))
            if status != "OK":
                return ExportResult(ok=False, partial_path=None,
                                    reason=f"LOG_BEGIN 返回 {status}（停车且日志收尾完成才允许）")
            transfer_id = begin.get("transfer_id")
            total_bytes = begin.get("total_bytes")
            record_count = begin.get("record_count")
            if begin.get("header_size") != records.LOG_HEADER_SIZE \
                    or begin.get("record_size") != records.RECORD_SIZE:
                return ExportResult(ok=False, reason="LOG_BEGIN 头/记录尺寸与 datalog v2 不符")
            if not isinstance(total_bytes, int) or not isinstance(record_count, int) \
                    or total_bytes != records.LOG_HEADER_SIZE + record_count * records.RECORD_SIZE:
                return ExportResult(
                    ok=False,
                    reason=f"total_bytes={total_bytes} 与 256+96*{record_count} 不一致")
            try:
                out = open(partial_path, "wb")
            except OSError as exc:
                return ExportResult(ok=False, reason=f"打开 .partial 失败：{exc}",
                                    partial_path=partial_path)
            offset = 0
            try:
                with out:
                    while offset < total_bytes:
                        length = min(LOG_READ_MAX_LENGTH, total_bytes - offset)
                        block = self.client.request_log_data(
                            "LOG_READ",
                            {"transfer_id": transfer_id, "offset": offset,
                             "length": length})
                        if block.offset < offset:
                            # 可靠重发的重复块：验证一致后跳过，不重写、不重复累计 CRC
                            overlap_end = block.offset + len(block.data)
                            if overlap_end <= offset:
                                continue
                            return self._fail(
                                partial_path, written, crc,
                                f"LOG_DATA 部分重叠块 offset={block.offset}，期望 {offset}")
                        if block.offset != offset:
                            return self._fail(
                                partial_path, written, crc,
                                f"LOG_DATA 错偏移：收到 {block.offset}，期望 {offset}")
                        if len(block.data) != length:
                            return self._fail(
                                partial_path, written, crc,
                                f"LOG_DATA 长度 {len(block.data)} 与请求 {length} 不符")
                        out.write(block.data)
                        crc = zlib.crc32(block.data, crc)
                        offset += len(block.data)
                        written = offset
                        if on_progress is not None:
                            on_progress(offset, total_bytes)
                # 文件内容先核对头，再 LOG_END 核对整体
                with open(partial_path, "rb") as f:
                    head = f.read(records.LOG_HEADER_SIZE)
                if head != meta.header_bytes:
                    return self._fail(partial_path, written, crc,
                                      "导出内容与 GET_LOG_META 缓存头不一致")
                records.validate_log_header(head)
                end = self.client.request_final("LOG_END",
                                                {"transfer_id": transfer_id})
                status = str(end.get("status", ""))
                if status != "OK":
                    return self._fail(partial_path, written, crc,
                                      f"LOG_END 返回 {status}")
                if end.get("total_bytes") != total_bytes:
                    return self._fail(partial_path, written, crc,
                                      "LOG_END total_bytes 与 LOG_BEGIN 不一致")
                device_crc = end.get("crc32")
                if not isinstance(device_crc, int) \
                        or (device_crc & 0xFFFFFFFF) != (crc & 0xFFFFFFFF):
                    return self._fail(
                        partial_path, written, crc,
                        f"整体 CRC32 不一致：本机 0x{crc & 0xFFFFFFFF:08X}，"
                        f"设备 {device_crc!r}")
            except client_mod.ClientError as exc:
                return self._fail(partial_path, written, crc, f"导出中断：{exc}")
            # 全部核对通过：改正式名并生成 57 列 CSV
            os.replace(partial_path, final_path)
            reason_path = partial_path + ".reason.txt"
            if os.path.exists(reason_path):
                os.remove(reason_path)
            # 导出 CSV：固定三行前导（#log_version/#black/#white），不加 #note
            convert_log(final_path, csv_path, header=meta.header,
                        include_notes=False)
            self._log({"event": "export_done", "file": final_path,
                       "total_bytes": total_bytes, "record_count": record_count,
                       "crc32": crc & 0xFFFFFFFF})
            return ExportResult(ok=True, final_path=final_path, csv_path=csv_path,
                                total_bytes=total_bytes, record_count=record_count,
                                crc32=crc & 0xFFFFFFFF)
        except client_mod.ClientError as exc:
            return ExportResult(ok=False, reason=str(exc))
        except KeyboardInterrupt:
            # 正常取消：尽力结束导出（设备超时负责最终释放互斥），保留 .partial
            self._log({"event": "export_cancelled", "partial": partial_path})
            return ExportResult(ok=False, reason="用户取消", partial_path=partial_path,
                                total_bytes=written, crc32=crc & 0xFFFFFFFF)

    def _fail(self, partial_path: str, written: int, crc: int,
              reason: str) -> ExportResult:
        """失败：保留 .partial 并写明原因文件。"""
        try:
            with open(partial_path + ".reason.txt", "w", encoding="utf-8") as f:
                f.write(reason + "\n")
        except OSError:
            pass
        self._log({"event": "export_failed", "reason": reason,
                   "partial": partial_path, "bytes_written": written})
        return ExportResult(ok=False, reason=reason, partial_path=partial_path,
                            total_bytes=written, crc32=crc & 0xFFFFFFFF)


# ---------------------------------------------------------------------------
# 离线转换
# ---------------------------------------------------------------------------

def convert_log(binary_path: str, csv_path: str,
                events_path: str | None = None,
                header: records.LogHeader | None = None,
                include_notes: bool = True) -> dict:
    """完整二进制日志 → 57 列 CSV（含 #log_version/#black/#white 前导）。

    events_path 提供 events.jsonl 时纳入参数历史提示；没有事件文件且
    include_notes=True 时在 CSV 中以 #note 行明确注明参数历史不完整
    （导出流程生成的 CSV 固定为三行前导，不加 #note）。
    返回转换信息 dict。
    """
    with open(binary_path, "rb") as f:
        data = f.read()
    if len(data) < records.LOG_HEADER_SIZE:
        raise ExportError(f"日志文件 {binary_path} 不足 256B 头")
    if (len(data) - records.LOG_HEADER_SIZE) % records.RECORD_SIZE != 0:
        raise ExportError(f"日志文件长度 {len(data)} 不满足 256+n*96")
    header_bytes = data[:records.LOG_HEADER_SIZE]
    if header is None:
        header = records.validate_log_header(header_bytes)
    count = (len(data) - records.LOG_HEADER_SIZE) // records.RECORD_SIZE
    notes = []
    if include_notes:
        if events_path is None:
            notes.append("无 events.jsonl：参数历史不完整，不能还原运行调参过程")
        elif not os.path.exists(events_path):
            notes.append(f"事件文件 {events_path} 不存在：参数历史不完整")
    with open(csv_path, "w", encoding="utf-8", newline="") as f:
        for line in records.csv_preamble_lines(header):
            f.write(line + "\n")
        for note in notes:
            f.write("#note," + note + "\n")
        f.write(records.csv_header_line() + "\n")
        for i in range(count):
            start = records.LOG_HEADER_SIZE + i * records.RECORD_SIZE
            record = records.unpack_record(data[start:start + records.RECORD_SIZE])
            f.write(",".join(records.record_to_csv_row(
                record, header.black, header.white)) + "\n")
    return {"csv_path": csv_path, "record_count": count,
            "mode": header.mode, "gear": header.gear, "notes": notes}
