#!/usr/bin/env python3
"""智能车 SCB/1 电脑端工具入口。

用法示例：
    python smartcar_link.py monitor --output run01
    python smartcar_link.py tune enable
    python smartcar_link.py tune set --kp 2 --k2p 0.008 --kd 15 --speed 180
    python smartcar_link.py tune disable
    python smartcar_link.py params get
    python smartcar_link.py dump --output run01-full
    python smartcar_link.py convert run01-full.bin --events run01.events.jsonl
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from scb_cli import main

if __name__ == "__main__":
    sys.exit(main())
