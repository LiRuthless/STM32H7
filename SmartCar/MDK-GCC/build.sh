#!/bin/bash
# SmartCar GCC 构建脚本（Git Bash 执行）：产出 MDK-GCC/SmartCar.{elf,hex,bin}
# 输出规范：默认安静（只显示文件名进度），有警告/错误时完整打印并汇总，
# 结束给出体积统计与明确成败。任何一步失败即中止（set -e）。
set -e
cd "$(dirname "$0")"
PROJ=".."
GCC="/c/Users/Li/.eide/tools/gcc_arm/bin/arm-none-eabi-gcc"
OBJCPY="/c/Users/Li/.eide/tools/gcc_arm/bin/arm-none-eabi-objcopy"
SIZE="/c/Users/Li/.eide/tools/gcc_arm/bin/arm-none-eabi-size"

INC=(-I"$PROJ/Core/Inc" -I"$PROJ/Drivers/STM32H7xx_HAL_Driver/Inc" \
     -I"$PROJ/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy" \
     -I"$PROJ/Drivers/CMSIS/Device/ST/STM32H7xx/Include" \
     -I"$PROJ/Drivers/CMSIS/Include" -I"$PROJ/App" -I"$PROJ/BSP" -I"$PROJ/BSP/lcd")
CFLAGS=(-mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 -O2 -g \
        -ffunction-sections -fdata-sections -std=c99 -Wall \
        -DUSE_HAL_DRIVER -DSTM32H743xx)

mkdir -p build
objs=()
warn_count=0
compile() {
  local src="$1"
  local obj="build/$(echo "${src#../}" | tr '/.' '__').o"
  local log="$obj.log"
  printf "  CC  %s\n" "${src#../}"
  if ! "$GCC" "${CFLAGS[@]}" "${INC[@]}" -c "$src" -o "$obj" > "$log" 2>&1; then
    echo "==================== 编译失败：$src ===================="
    cat "$log"
    exit 1
  fi
  if grep -qE "warning|error" "$log"; then
    warn_count=$((warn_count + 1))
    echo "  ---- 警告（$src）:"
    grep -E "warning|error" "$log"
  fi
  objs+=("$obj")
}

echo "=== [1/3] 编译 ==="
for f in "$PROJ"/Core/Src/*.c "$PROJ"/Drivers/STM32H7xx_HAL_Driver/Src/*.c \
         "$PROJ"/BSP/*.c "$PROJ"/BSP/lcd/*.c "$PROJ"/App/*.c; do
  compile "$f"
done
compile startup_stm32h743xx.s

echo "=== [2/3] 链接 ==="
"$GCC" "${CFLAGS[@]}" -T STM32H743VITx.ld -nostartfiles \
  --specs=nano.specs --specs=nosys.specs \
  -Wl,-Map=build/SmartCar.map,--gc-sections \
  "${objs[@]}" -o SmartCar.elf

echo "=== [3/3] 生成 hex/bin ==="
"$OBJCPY" -O ihex   SmartCar.elf SmartCar.hex
"$OBJCPY" -O binary SmartCar.elf SmartCar.bin
"$SIZE" SmartCar.elf

if [ "$warn_count" -gt 0 ]; then
  echo "*** 完成（有 $warn_count 个文件带警告，见上方）***"
else
  echo "*** 完成：0 警告 0 错误，产物 MDK-GCC/SmartCar.hex ***"
fi
