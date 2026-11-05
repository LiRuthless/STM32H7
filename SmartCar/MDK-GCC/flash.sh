#!/bin/bash
# 兼容旧调用：统一使用工作区便携 OpenOCD，默认烧录 Keil HEX，不自动构建。
set -e
workspace_root="$(cd "$(dirname "$0")/../.." && pwd)"
powershell_exe="$(cygpath -u "${SYSTEMROOT:-C:\\Windows}")/System32/WindowsPowerShell/v1.0/powershell.exe"
exec "$powershell_exe" -NoProfile -ExecutionPolicy Bypass \
  -File "$(cygpath -w "$workspace_root/Tools/STM32Flash/flash.ps1")" "$@"
