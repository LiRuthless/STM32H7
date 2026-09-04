#!/bin/bash
# SmartCar GCC 构建脚本（Git Bash 执行）：产出 MDK-GCC/SmartCar.{elf,hex,bin}
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
compile() {
  local src="$1"
  local obj="build/$(echo "${src#../}" | tr '/.' '__').o"
  "$GCC" "${CFLAGS[@]}" "${INC[@]}" -c "$src" -o "$obj"
  objs+=("$obj")
}

for f in "$PROJ"/Core/Src/*.c "$PROJ"/Drivers/STM32H7xx_HAL_Driver/Src/*.c \
         "$PROJ"/BSP/*.c "$PROJ"/BSP/lcd/*.c "$PROJ"/App/*.c; do
  compile "$f"
done
compile startup_stm32h743xx.s

"$GCC" "${CFLAGS[@]}" -T STM32H743VITx.ld -nostartfiles \
  --specs=nano.specs --specs=nosys.specs \
  -Wl,-Map=build/SmartCar.map,--gc-sections \
  "${objs[@]}" -o SmartCar.elf

"$OBJCPY" -O ihex   SmartCar.elf SmartCar.hex
"$OBJCPY" -O binary SmartCar.elf SmartCar.bin
"$SIZE" SmartCar.elf
echo "BUILD OK: MDK-GCC/SmartCar.hex"
