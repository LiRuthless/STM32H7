#!/bin/bash
# SmartCar 一键构建+烧录（Git Bash 执行）：GCC 编译 → OpenOCD/ST-Link 烧录并校验
# 前提：ST-Link 已接 SWD 排针（3V3/SWDIO/SWCLK/GND），目标板已供电
set -e
cd "$(dirname "$0")"

echo "=== [1/2] GCC 构建 ==="
bash build.sh

echo "=== [2/2] OpenOCD 烧录 ==="
"/c/Users/Li/.eide/tools/openocd_7a1adfbec_mingw32/bin/openocd" \
  -f interface/stlink.cfg -f target/stm32h7x.cfg \
  -c "program SmartCar.hex verify reset exit"

echo "=== 完成：已烧录并复位运行 ==="
