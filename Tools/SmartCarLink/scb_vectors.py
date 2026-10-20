"""SCB/1 三端共同测试向量（固定十六进制）。

字节串由 scb_protocol/scb_records 一次性构造后硬编码于此（生成脚本已删除），
此后视为固定外部数据：单元测试用 scb_protocol 独立解码这些固定字节，
并与本文件中的期望字段逐项比对（独立交叉核对，不是同一编解码器自我往返）。

覆盖任务书第 7 节：IDLE、GET_STATUS、完整 SET_PARAMS、1 条/10 条记录
TELEMETRY、LOG_DATA、错误 CRC、错误长度、重复可靠帧、重复 request_id。
"""

from __future__ import annotations

import scb_protocol as protocol

# 向量统一使用的会话与运行编号
VECTOR_SESSION_ID = 0xA5C30001
VECTOR_RUN_ID = 7

# ---------------------------------------------------------------------------
# 固定向量字节（十六进制）
# ---------------------------------------------------------------------------

# IDLE：空负载、非可靠、seq=9、ack_seq=4
IDLE_HEX = (
    "5343423101030000000000000100c3a5090000000400000000000000a1ac4089"
)

# GET_STATUS COMMAND：RELIABLE、seq=1、request_id=1、{"op":"GET_STATUS","args":{}}
GET_STATUS_HEX = (
    "53434231011001001d0000000100c3a50100000000000000010000001ca9ef94"
    "7b226f70223a224745545f535441545553222c2261726773223a7b7d7d"
)

# SET_PARAMS COMMAND：完整 8 项 values、seq=2、request_id=2
SET_PARAMS_HEX = (
    "5343423101100100a00000000100c3a5020000000000000002000000129cef98"
    "7b226f70223a225345545f504152414d53222c2261726773223a7b226d6f6465"
    "223a312c2267656172223a302c2276616c756573223a7b226b70223a322e302c"
    "226b3270223a302e3030382c226b64223a31352e302c226b705f76223a32302e"
    "302c226b695f76223a302e37352c22626173655f7370656564223a3138302c22"
    "66616e5f72756e223a313630302c2266616e5f69646c65223a313130307d7d7d"
)

# TELEMETRY 1 条记录：record_count=1、payload_len=120、seq=3
TELEMETRY_1_HEX = (
    "5343423101200000780001000100c3a50300000000000000000000006132aa5d"
    "00000000030000000000000000000000e803000007000000e8030000640038ff"
    "2c0170fe0a00f5ffceff3c000100feff03000020e4020201000007000e001500"
    "1c0023002a00310038003f0046004d0054005b00620069008813000070170000"
    "d4fe90010500010301000000000000000000000000000000"
)

# TELEMETRY 10 条记录：record_count=10、payload_len=984、seq=4，
# 记录索引 10~19 连续，tick 1010~1019 与元数据 first_tick 一致
TELEMETRY_10_HEX = (
    "5343423101200000d8030a000100c3a5040000000000000000000000ba378dc5"
    "0a000000030000000200000001000000f203000007000000f2030000640038ff"
    "2c0170fe0a00f5ffceff3c000100feff03000020e4020201000007000e001500"
    "1c0023002a00310038003f0046004d0054005b00620069008813000070170000"
    "d4fe90010500010301000000000000000000000000000000f3030000650037ff"
    "2d016ffe0b00f4ffcdff3d000200fdff04000020e5020201700077007e008500"
    "8c0093009a00a100a800af00b600bd00c400cb00d200d9008913000071170000"
    "d3fe91010500010301000000000000000000000000000000f4030000660036ff"
    "2e016efe0c00f3ffccff3e000300fcff05000020e6020201e000e700ee00f500"
    "fc0003010a01110118011f0126012d0134013b01420149018a13000072170000"
    "d2fe92010500010301000000000000000000000000000000f5030000670035ff"
    "2f016dfe0d00f2ffcbff3f000400fbff06000020e7020201500157015e016501"
    "6c0173017a01810188018f0196019d01a401ab01b201b9018b13000073170000"
    "d1fe93010500010301000000000000000000000000000000f6030000680034ff"
    "30016cfe0e00f1ffcaff40000500faff07000020e8020201c001c701ce01d501"
    "dc01e301ea01f101f801ff0106020d0214021b02220229028c13000074170000"
    "d0fe94010500010301000000000000000000000000000000f7030000690033ff"
    "31016bfe0f00f0ffc9ff41000600f9ff08000020e9020201300237023e024502"
    "4c0253025a02610268026f0276027d0284028b02920299028d13000075170000"
    "cffe95010500010301000000000000000000000000000000f80300006a0032ff"
    "32016afe1000efffc8ff42000700f8ff09000020ea020201a002a702ae02b502"
    "bc02c302ca02d102d802df02e602ed02f402fb02020309038e13000076170000"
    "cefe96010500010301000000000000000000000000000000f90300006b0031ff"
    "330169fe1100eeffc7ff43000800f7ff0a000020eb020201100317031e032503"
    "2c0333033a03410348034f0356035d0364036b03720379038f13000077170000"
    "cdfe97010500010301000000000000000000000000000000fa0300006c0030ff"
    "340168fe1200edffc6ff44000900f6ff0b000020ec020201800387038e039503"
    "9c03a303aa03b103b803bf03c603cd03d403db03e203e9039013000078170000"
    "ccfe98010500010301000000000000000000000000000000fb0300006d002fff"
    "350167fe1300ecffc5ff45000a00f5ff0c000020ed020201f003f703fe030504"
    "0c0413041a04210428042f0436043d0444044b04520459049113000079170000"
    "cbfe99010500010301000000000000000000000000000000"
)

# LOG_DATA：RELIABLE、seq=5、ack_seq=2、request_id=6、offset=256、32B 数据 0x00~0x1F
LOG_DATA_HEX = (
    "5343423101300100280000000100c3a50500000002000000060000004a360581"
    "0001000020000000000102030405060708090a0b0c0d0e0f1011121314151617"
    "18191a1b1c1d1e1f"
)

# 错误 CRC：GET_STATUS 帧的 CRC 字段最低位翻转
BAD_CRC_HEX = (
    "53434231011001001d0000000100c3a50100000000000000010000001da9ef94"
    "7b226f70223a224745545f535441545553222c2261726773223a7b7d7d"
)

# 错误长度：IDLE 帧头 payload_len 被改为 100，实际无负载
BAD_LENGTH_HEX = (
    "5343423101030000640000000100c3a5090000000400000000000000a1ac4089"
)

# 重复可靠帧：同 seq=7、同 request_id=9、同内容两帧（可靠重发保留原 seq 和原内容）
DUP_RELIABLE_HEX = (
    "53434231011001001d0000000100c3a5070000000000000009000000c041e8e5"
    "7b226f70223a224745545f535441545553222c2261726773223a7b7d7d",
    "53434231011001001d0000000100c3a5070000000000000009000000c041e8e5"
    "7b226f70223a224745545f535441545553222c2261726773223a7b7d7d",
)

# 重复 request_id=11：同编号同内容两帧（TUNE_ENABLE）+ 同编号不同内容一帧（TUNE_DISABLE）
DUP_REQUEST_ID_HEX = (
    "53434231011001001e0000000100c3a50a000000000000000b000000411c8c70"
    "7b226f70223a2254554e455f454e41424c45222c2261726773223a7b7d7d",
    "53434231011001001e0000000100c3a50a000000000000000b000000411c8c70"
    "7b226f70223a2254554e455f454e41424c45222c2261726773223a7b7d7d",
    "53434231011001001f0000000100c3a50c000000000000000b000000ff29dcde"
    "7b226f70223a2254554e455f44495341424c45222c2261726773223a7b7d7d",
)

# ---------------------------------------------------------------------------
# 期望字段（独立核对用）
# ---------------------------------------------------------------------------

# SET_PARAMS 完整 8 项 values 的期望内容
SET_PARAMS_EXPECTED_VALUES = {
    "kp": 2.0,
    "k2p": 0.008,
    "kd": 15.0,
    "kp_v": 20.0,
    "ki_v": 0.75,
    "base_speed": 180,
    "fan_run": 1600,
    "fan_idle": 1100,
}

# 各正常向量的期望帧头字段（type/flags/payload_len/record_count/seq/ack_seq/request_id）
EXPECTED_HEADERS = {
    "IDLE": dict(msg_type=protocol.MSG_IDLE, flags=0, payload_len=0,
                 record_count=0, seq=9, ack_seq=4, request_id=0),
    "GET_STATUS": dict(msg_type=protocol.MSG_COMMAND, flags=protocol.FLAG_RELIABLE,
                       payload_len=29, record_count=0, seq=1, ack_seq=0, request_id=1),
    "SET_PARAMS": dict(msg_type=protocol.MSG_COMMAND, flags=protocol.FLAG_RELIABLE,
                       payload_len=160, record_count=0, seq=2, ack_seq=0, request_id=2),
    "TELEMETRY_1": dict(msg_type=protocol.MSG_TELEMETRY, flags=0,
                        payload_len=120, record_count=1, seq=3, ack_seq=0,
                        request_id=0),
    "TELEMETRY_10": dict(msg_type=protocol.MSG_TELEMETRY, flags=0,
                         payload_len=984, record_count=10, seq=4, ack_seq=0,
                         request_id=0),
    "LOG_DATA": dict(msg_type=protocol.MSG_LOG_DATA, flags=protocol.FLAG_RELIABLE,
                     payload_len=40, record_count=0, seq=5, ack_seq=2, request_id=6),
}

# TELEMETRY 元数据期望
TELEMETRY_1_META = dict(first_index=0, revision=3, stm32_dropped_records=0,
                        esp32_dropped_packets=0, first_tick=1000, run_id=VECTOR_RUN_ID)
TELEMETRY_10_META = dict(first_index=10, revision=3, stm32_dropped_records=2,
                         esp32_dropped_packets=1, first_tick=1010, run_id=VECTOR_RUN_ID)


def expected_record_fields(i: int, base_tick: int = 1000) -> dict:
    """第 i 条测试记录的期望字段（与生成时的确定性公式一致，独立于解码路径）。

    TELEMETRY_1 使用 base_tick=1000；TELEMETRY_10 使用 base_tick=1010。
    """
    return dict(
        tick=base_tick + i,
        adc=(100 + i, -200 - i, 300 + i, -400 - i),
        speed_l=10 + i,
        speed_r=-(11 + i),
        track_error=-50 - i,
        track_out=60 + i,
        gyro_x=1 + i,
        gyro_y=-(2 + i),
        gyro_z=3 + i,
        dl1b_mm=8192,
        battery=740 + i,
        kernel_state=2,
        roundabout_state=1,
        gray_raw=tuple((i * 16 + j) * 7 % 4096 for j in range(16)),
        gray_sequence=5000 + i,
        control_sequence=6000 + i,
        em_error=-300 - i,
        optical_error=400 + i,
        protection_flags=0x0005,
        mode=1,
        optical_state=3,
        gray_valid=1,
        stop_reason=0,
        reserved=b"\x00" * 14,
    )


def from_hex(hex_text: str) -> bytes:
    """固定向量十六进制转字节。"""
    return bytes.fromhex(hex_text)
