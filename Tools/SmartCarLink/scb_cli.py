"""SCB/1 命令行入口。

子命令：
    monitor   常驻连接 + 实时记录 + 本地命令接口
    tune      enable / set / disable
    params    get
    dump      停车日志完整导出
    convert   离线二进制日志 → 57 列 CSV

公共选项：--host/--port/--udp-port/--timeout（默认 192.168.4.1:5000、UDP 5001、5s）。
加 --monitor 时把命令转交给本机 monitor（127.0.0.1:15000）；
没有 monitor 时单次 CLI 自行连接设备，不偷偷启动第二个设备会话。
"""

from __future__ import annotations

import argparse
import json
import sys

import scb_client as client_mod
import scb_export as export_mod
import scb_monitor as monitor_mod
import scb_tune as tune_mod


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="smartcar_link.py",
        description="智能车 SCB/1 电脑端工具（Python 3.10+ 标准库，无第三方依赖）")
    parser.add_argument("--host", default=client_mod.DEFAULT_HOST,
                        help="设备地址（默认 %(default)s）")
    parser.add_argument("--port", type=int, default=client_mod.DEFAULT_TCP_PORT,
                        help="设备 TCP 端口（默认 %(default)s）")
    parser.add_argument("--udp-port", type=int, default=client_mod.DEFAULT_UDP_PORT,
                        help="本机 UDP 接收端口（默认 %(default)s）")
    parser.add_argument("--timeout", type=float,
                        default=client_mod.DEFAULT_TIMEOUT_S,
                        help="请求超时秒数（默认 %(default)s）")
    parser.add_argument("--monitor", action="store_true",
                        help="转交给本机 monitor（127.0.0.1:15000），不直接连设备")
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("monitor", help="常驻连接设备并实时记录")
    p.add_argument("--output", default="run",
                   help="输出文件基名（默认 %(default)s）")

    p = sub.add_parser("tune", help="运行调参")
    tune_sub = p.add_subparsers(dest="tune_command", required=True)
    tune_sub.add_parser("enable", help="开启当前目标的普通赛道覆盖")
    pset = tune_sub.add_parser(
        "set", help="整组提交参数（先取设备完整值，只替换指定项）",
        description="base_speed 单位：脉冲/2ms；风扇为万分比。不静默钳位。")
    pset.add_argument("--kp", type=float, help="电磁/光电主增益（有限数）")
    pset.add_argument("--k2p", type=float, help="二次项增益（有限数）")
    pset.add_argument("--kd", type=float, help="微分增益（有限数）")
    pset.add_argument("--kp_v", type=float, help="速度比例（有限数）")
    pset.add_argument("--ki_v", type=float, help="速度积分（有限数）")
    pset.add_argument("--speed", type=int,
                      help="base_speed，单位：脉冲/2ms（整数）")
    pset.add_argument("--fan_run", type=int, help="运行风扇，万分比（整数）")
    pset.add_argument("--fan_idle", type=int, help="怠速风扇，万分比（整数）")
    tune_sub.add_parser("disable", help="关闭覆盖并恢复基线")

    p = sub.add_parser("params", help="参数查询")
    params_sub = p.add_subparsers(dest="params_command", required=True)
    pget = params_sub.add_parser("get", help="GET_PARAMS（默认读 GET_STATUS 当前目标）")
    pget.add_argument("--mode", type=int, choices=[0, 1], default=None,
                      help="0 电磁 / 1 光电（默认取设备当前）")
    pget.add_argument("--gear", type=int, choices=[0, 1], default=None,
                      help="0 低档 / 1 高档（默认取设备当前）")

    p = sub.add_parser("dump", help="停车日志完整导出（停车且日志收尾完成后）")
    p.add_argument("--output", default="run-full",
                   help="输出文件基名（默认 %(default)s）")

    p = sub.add_parser("convert", help="离线转换：完整二进制日志 → 57 列 CSV")
    p.add_argument("input", help="完整二进制日志文件（.bin）")
    p.add_argument("--output", default=None, help="CSV 路径（默认 <输入>.csv）")
    p.add_argument("--events", default=None,
                   help="可选 events.jsonl；缺失时注明参数历史不完整")
    return parser


def _print_json(obj) -> None:
    print(json.dumps(obj, ensure_ascii=False, indent=2, default=str))


def _collect_overrides(args) -> dict:
    overrides = {}
    for cli_key, key in tune_mod.CLI_KEY_MAP.items():
        value = getattr(args, cli_key, None)
        if value is not None:
            overrides[key] = value
    return overrides


def _run_with_client(args, fn) -> int:
    """单次 CLI：自行连接设备（不偷偷起第二个会话），结束后断开。"""
    client = client_mod.DeviceClient(host=args.host, port=args.port,
                                     udp_port=args.udp_port,
                                     timeout=args.timeout)
    try:
        client.connect()
    except client_mod.ClientError as exc:
        print(f"连接设备失败：{exc}", file=sys.stderr)
        return 2
    try:
        client.recover_pending()  # 重连先查询未决请求，不重放
        return fn(client)
    except client_mod.ClientError as exc:
        print(f"失败：{exc}", file=sys.stderr)
        return 1
    finally:
        client.close()


def _via_monitor(args, op: str, op_args: dict) -> int:
    try:
        response = monitor_mod.forward_to_monitor(op, op_args)
    except monitor_mod.MonitorError as exc:
        print(f"转交 monitor 失败：{exc}", file=sys.stderr)
        return 2
    _print_json(response)
    return 0 if response.get("status") in ("OK", "APPLIED") else 1


def main(argv=None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.command == "monitor":
        monitor = monitor_mod.Monitor(host=args.host, port=args.port,
                                      udp_port=args.udp_port,
                                      output_base=args.output,
                                      timeout=args.timeout)
        try:
            return monitor.run()
        except monitor_mod.MonitorError as exc:
            print(f"monitor 启动失败：{exc}", file=sys.stderr)
            return 2

    if args.command == "convert":
        output = args.output or (args.input + ".csv")
        try:
            info = export_mod.convert_log(args.input, output,
                                          events_path=args.events)
        except export_mod.ExportError as exc:
            print(f"转换失败：{exc}", file=sys.stderr)
            return 1
        for note in info["notes"]:
            print(f"注意：{note}")
        print(f"已生成 {info['csv_path']}（{info['record_count']} 条记录，"
              f"mode={info['mode']} gear={info['gear']}）")
        return 0

    if args.command == "tune":
        if args.tune_command == "enable":
            if args.monitor:
                return _via_monitor(args, "TUNE_ENABLE", {})
            return _run_with_client(args, _do_tune_enable)
        if args.tune_command == "disable":
            if args.monitor:
                return _via_monitor(args, "TUNE_DISABLE", {})
            return _run_with_client(args, _do_tune_disable)
        if args.tune_command == "set":
            overrides = _collect_overrides(args)
            if not overrides:
                print("tune set 至少需要一个参数项", file=sys.stderr)
                return 2
            if args.monitor:
                return _via_monitor_set(args, overrides)
            return _run_with_client(
                args, lambda c: _do_tune_set(c, overrides))

    if args.command == "params":
        if args.monitor:
            op_args = {}
            if args.mode is not None:
                op_args["mode"] = args.mode
            if args.gear is not None:
                op_args["gear"] = args.gear
            return _via_monitor(args, "GET_PARAMS", op_args)
        return _run_with_client(args, lambda c: _do_params_get(c, args))

    if args.command == "dump":
        return _run_with_client(args, lambda c: _do_dump(c, args))

    return 2


def _do_tune_enable(client) -> int:
    outcome = tune_mod.tune_enable(client)
    result = outcome["result"]
    if outcome["ok"]:
        print(f"TUNE_ENABLE OK：实际 tick={result.get('tick')}，"
              f"控制拍={result.get('control_sequence')}")
        return 0
    print(f"TUNE_ENABLE 被拒：{result.get('status')}（实际许可由 STM32 决定）")
    _print_json(result)
    return 1


def _do_tune_disable(client) -> int:
    outcome = tune_mod.tune_disable(client)
    if outcome["ok"]:
        print("TUNE_DISABLE OK：覆盖已关闭并恢复基线")
        return 0
    _print_json(outcome["result"])
    return 1


def _do_tune_set(client, overrides: dict) -> int:
    outcome = tune_mod.tune_set(client, overrides)
    status = outcome["status"]
    if outcome["ok"]:
        print(f"SET_PARAMS APPLIED（已生效）：revision={outcome.get('revision')}，"
              f"tick={outcome.get('tick')}，控制拍={outcome.get('control_sequence')}")
        print("实际完整 values：" + tune_mod.format_values(
            outcome.get("applied_values") or {}))
        print("注意：APPLIED 表示已生效；持久化以 PARAM_SAVED/SAVED 事件为准")
        return 0
    print(f"SET_PARAMS 未生效：{status}（RECEIVED 不等于生效）")
    _print_json(outcome.get("result"))
    return 1


def _do_params_get(client, args) -> int:
    if args.mode is None or args.gear is None:
        mode, gear, _ = tune_mod.get_current_target(client)
        mode = args.mode if args.mode is not None else mode
        gear = args.gear if args.gear is not None else gear
    else:
        mode, gear = args.mode, args.gear
    result = client.request_final("GET_PARAMS", {"mode": mode, "gear": gear})
    if result.get("status") != "OK":
        _print_json(result)
        return 1
    print(f"mode={mode}({tune_mod.MODE_NAMES[mode]}) "
          f"gear={gear}({tune_mod.GEAR_NAMES[gear]})")
    print("values：" + tune_mod.format_values(result.get("values") or {}))
    print("limits：")
    _print_json(result.get("limits"))
    return 0


def _do_dump(client, args) -> int:
    exporter = export_mod.LogExporter(client)
    print("正在导出停车日志（窗口 1 顺序读，可能约 87s 数据）……")
    result = exporter.export(
        args.output,
        on_progress=lambda done, total: print(
            f"\r{done}/{total} 字节", end="", flush=True))
    print()
    if result.ok:
        print(f"导出完成：{result.final_path}（{result.total_bytes} 字节，"
              f"{result.record_count} 条记录，CRC32=0x{result.crc32:08X}）")
        print(f"CSV：{result.csv_path}")
        return 0
    print(f"导出失败：{result.reason}", file=sys.stderr)
    if result.partial_path:
        print(f"已保留不完整文件：{result.partial_path}", file=sys.stderr)
    return 1


def _via_monitor_set(args, overrides: dict) -> int:
    # monitor 转发暂不支持整组拼装（需先查设备 values/limits），
    # 提示直接连设备执行，避免绕过整组校验。
    print("tune set 需要读取设备当前 values/limits 后整组提交，"
          "请去掉 --monitor 直接连设备执行", file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main())
