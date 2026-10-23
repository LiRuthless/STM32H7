from pathlib import Path

out = Path(__file__).resolve().parent / "command-capture"
out.mkdir(exist_ok=True)
template = Path(__file__).resolve().parents[1] / "stm32-diagnosis.cfg"
config = template.read_text().split('echo [targets]')[0]
config += '''echo [stm32h7x.cpu0 curstate]
for {set i 0} {$i < 150} {incr i} {
    echo "CAP $i TICK [stm32h7x.cpu0 read_memory 0x24000008 32 1] BEFORE [stm32h7x.cpu0 read_memory 0x24034924 32 6] RX [stm32h7x.cpu0 read_memory 0x2407f400 32 32] AFTER [stm32h7x.cpu0 read_memory 0x24034924 32 6] LINK [stm32h7x.cpu0 read_memory 0x24008f28 32 6]"
    sleep 10
}
echo [stm32h7x.cpu0 curstate]
shutdown
'''
(out / "capture.cfg").write_text(config)
