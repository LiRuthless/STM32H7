"""SCB/1 调参业务逻辑（参数拼装/校验与网络解耦，便于单测）。

规则（任务书第 4 节与 Python 实现 C 节）：
- SET_PARAMS 的 values 必须完整包含八项；tune set 先 GET_PARAMS 取完整 8 项
  及 limits，只替换用户指定项，再整组提交。
- 五项增益为有限数，速度/风扇为整数；按设备 limits 校验范围，不静默钳位。
- 运行时只允许当前 mode/gear，fan_idle 须等于当前有效值。
- RECEIVED 不是生效；APPLIED 才称已生效（含实际完整 values/revision/tick/
  control_sequence）；SAVED/PARAM_SAVED 才称已持久化。
- base_speed 单位：脉冲/2ms；风扇为万分比。
"""

from __future__ import annotations

import math
import time

import scb_client as client_mod

TUNE_VALUE_KEYS = ("kp", "k2p", "kd", "kp_v", "ki_v",
                   "base_speed", "fan_run", "fan_idle")
GAIN_KEYS = ("kp", "k2p", "kd", "kp_v", "ki_v")        # 有限数
INTEGER_KEYS = ("base_speed", "fan_run", "fan_idle")   # 整数
# CLI 选项名 → 参数键
CLI_KEY_MAP = {"kp": "kp", "k2p": "k2p", "kd": "kd", "kp_v": "kp_v",
               "ki_v": "ki_v", "speed": "base_speed",
               "fan_run": "fan_run", "fan_idle": "fan_idle"}

UNIT_NOTES = {"base_speed": "脉冲/2ms", "fan_run": "万分比", "fan_idle": "万分比"}

MODE_NAMES = {0: "电磁", 1: "光电"}
GEAR_NAMES = {0: "低档", 1: "高档"}


class TuneError(client_mod.ClientError):
    """调参输入/流程错误。"""


def validate_value(key: str, value, limits: dict | None) -> float | int:
    """校验单个参数：有限数/整数类型/设备范围。不静默钳位，越界即拒绝。"""
    if key not in TUNE_VALUE_KEYS:
        raise TuneError(f"未知参数 {key!r}；支持：{', '.join(TUNE_VALUE_KEYS)}")
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise TuneError(f"参数 {key} 必须为数，实际 {value!r}")
    if key in GAIN_KEYS:
        if not math.isfinite(value):
            raise TuneError(f"增益 {key} 必须为有限数，实际 {value!r}")
        checked: float | int = float(value)
    else:
        if not isinstance(value, int):
            raise TuneError(f"参数 {key} 必须为整数，实际 {value!r}")
        checked = value
    if limits is None or key not in limits:
        raise TuneError(f"设备未返回参数 {key} 的 limits，无法验证范围，拒绝提交")
    lo, hi = limits[key].get("min"), limits[key].get("max")
    if lo is None or hi is None:
        raise TuneError(f"设备返回的 {key} limits 不完整：{limits[key]!r}")
    if not (lo <= checked <= hi):
        unit = UNIT_NOTES.get(key, "")
        raise TuneError(
            f"参数 {key}={checked}{unit} 超出设备范围 [{lo}, {hi}]，不做钳位")
    return checked


def build_set_values(current_values: dict, overrides: dict,
                     limits: dict | None) -> dict:
    """以设备当前完整 values 为基础，只替换指定项，返回完整 8 项。

    current_values 必须已含完整 8 项；overrides 逐项校验。
    """
    missing = [k for k in TUNE_VALUE_KEYS if k not in current_values]
    if missing:
        raise TuneError(f"GET_PARAMS 未返回完整 values，缺少：{', '.join(missing)}")
    values = dict(current_values)
    for key, value in overrides.items():
        values[key] = validate_value(key, value, limits)
    return values


def get_current_target(client: client_mod.DeviceClient) -> tuple[int, int, dict]:
    """GET_STATUS 取当前 mode/gear 与运行状态。"""
    result = client.request_final("GET_STATUS", {})
    if result.get("status") != "OK":
        raise TuneError(f"GET_STATUS 返回 {result.get('status')}")
    mode, gear = result.get("mode"), result.get("gear")
    if mode not in (0, 1) or gear not in (0, 1):
        raise TuneError(f"GET_STATUS 返回非法 mode/gear：{mode!r}/{gear!r}")
    return mode, gear, result


def tune_set(client: client_mod.DeviceClient, overrides: dict,
             event_logger=None, timeout: float | None = None) -> dict:
    """tune set 全流程：查询 → 整组拼装 → 提交 → 等待 APPLIED/错误。

    返回 dict（ok/status/applied_values/revision/...）。APPLIED 才称已生效。
    """
    if not overrides:
        raise TuneError("tune set 至少需要一个参数项")
    mode, gear, status_result = get_current_target(client)
    params = client.request_final("GET_PARAMS", {"mode": mode, "gear": gear})
    if params.get("status") != "OK":
        raise TuneError(f"GET_PARAMS 返回 {params.get('status')}")
    values = build_set_values(params.get("values") or {}, overrides,
                              params.get("limits"))
    if status_result.get("running"):
        # 运行时：fan_idle 必须等于当前有效值（设备仍会最终裁决）
        current_fan_idle = (params.get("values") or {}).get("fan_idle")
        if values["fan_idle"] != current_fan_idle:
            raise TuneError(
                f"运行时 fan_idle 须等于当前值 {current_fan_idle}，"
                f"实际提交 {values['fan_idle']}")
    result = client.request_final(
        "SET_PARAMS", {"mode": mode, "gear": gear, "values": values},
        timeout=timeout)
    status = str(result.get("status", ""))
    request_id = result.get("request_id")
    event = {
        "event": "tune_set_result",
        "local_time_ns": time.time_ns(),
        "request_id": request_id,
        "session_id": client.session.session_id,
        "run_id": result.get("run_id"),
        "tick": result.get("tick"),
        "control_sequence": result.get("control_sequence"),
        "revision": result.get("revision"),
        "confirm_status": status,
        "requested_overrides": overrides,
        "actual_values": result.get("values"),
        "persisted": False,   # 只有 PARAM_SAVED/SAVED 事件才置真
    }
    if event_logger is not None:
        event_logger(event, immediate=True)  # 参数最终结果立即 flush
    if status != "APPLIED":
        return {"ok": False, "status": status, "result": result}
    return {"ok": True, "status": status, "applied_values": result.get("values"),
            "revision": result.get("revision"), "tick": result.get("tick"),
            "control_sequence": result.get("control_sequence"),
            "result": result}


def tune_enable(client: client_mod.DeviceClient, event_logger=None) -> dict:
    """tune enable：先查起停/元素状态（实际许可仍由 STM32 决定）。"""
    _, _, status_result = get_current_target(client)
    result = client.request_final("TUNE_ENABLE", {})
    _log_tune_event(client, event_logger, "tune_enable_result", result)
    return {"ok": result.get("status") == "OK",
            "precheck": {"running": status_result.get("running"),
                         "element": status_result.get("element")},
            "result": result}


def tune_disable(client: client_mod.DeviceClient, event_logger=None) -> dict:
    result = client.request_final("TUNE_DISABLE", {})
    _log_tune_event(client, event_logger, "tune_disable_result", result)
    return {"ok": result.get("status") == "OK", "result": result}


def _log_tune_event(client, event_logger, name, result) -> None:
    if event_logger is None:
        return
    event_logger({
        "event": name,
        "local_time_ns": time.time_ns(),
        "request_id": result.get("request_id"),
        "session_id": client.session.session_id,
        "tick": result.get("tick"),
        "control_sequence": result.get("control_sequence"),
        "revision": result.get("revision"),
        "confirm_status": result.get("status"),
        "actual_values": result.get("values"),
        "persisted": False,
    }, immediate=True)


def format_values(values: dict) -> str:
    """展示完整 8 项（带单位说明）。"""
    parts = []
    for key in TUNE_VALUE_KEYS:
        unit = UNIT_NOTES.get(key, "")
        suffix = f"({unit})" if unit else ""
        parts.append(f"{key}={values.get(key)}{suffix}")
    return ", ".join(parts)
