"""Independent before/after GCC resource evidence without rewriting old results.

Baseline is an exact git archive. Current configuration and generated products
are restored in finally, including failure. Stack numbers are planning evidence,
not a measured watermark or a formal upper bound.
"""
import argparse
from collections import Counter
import datetime
import hashlib
import io
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import time

REPO = Path(__file__).resolve().parents[2]
ROOT = REPO/'SmartCar'
ARM = Path('C:/Users/Li/.eide/tools/gcc_arm/bin')
BASH = Path('C:/Program Files/Git/bin/bash.exe')
BASE = '4745aa2339209d5df9a739449685fc7f1cb66429'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def command(args, log=None):
    result = subprocess.run(list(map(str, args)), cwd=REPO, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if log:
        log.write_bytes(result.stdout)
    if result.returncode:
        raise RuntimeError(result.stdout.decode('utf-8', errors='replace'))
    return result.stdout.decode('utf-8', errors='replace')


def configure(root, original, lcd):
    value, count = re.subn(rb'(#define LCD_TARGET_EXTERNAL )0\b',
                           lambda match: match[1]+str(lcd).encode(), original)
    assert count == 1, 'LCD configuration must start at original 0'
    (root/'App/app_config.h').write_bytes(value)


def restore_files(backup):
    failures = []
    for path, value in backup.items():
        for attempt in range(3):
            try:
                if value is None:
                    if path.exists():
                        path.unlink()
                else:
                    path.write_bytes(value)
                    assert path.read_bytes() == value, ('restore mismatch', path)
                break
            except (OSError, AssertionError) as error:
                if attempt == 2:
                    failures.append((str(path), str(error)))
                else:
                    time.sleep(0.2)
    if failures:
        raise RuntimeError(('Original files could not all be restored', failures))


def symbols(elf):
    output = command([ARM/'arm-none-eabi-nm.exe', '-S', '-n', elf])
    rows = []
    for line in output.splitlines():
        parts = line.split()
        if len(parts) == 4:
            rows.append(dict(name=parts[3], address=int(parts[0], 16),
                             size=int(parts[1], 16), section=parts[2]))
        elif len(parts) == 3:
            rows.append(dict(name=parts[2], address=int(parts[0], 16), size=0, section=parts[1]))
    for name, address, size in [('s_tx', 0x2407F000, 1024), ('s_rx', 0x2407F400, 1024),
                                ('s_adc_buf', 0x30000000, 20)]:
        assert [r for r in rows if r['name'] == name and r['address'] == address and r['size'] == size], name
    estack = [r['address'] for r in rows if r['name'] == '_estack']
    ebss = [r['address'] for r in rows if r['name'] == '_ebss']
    assert estack == [0x2407F000] and len(ebss) == 1 and ebss[0]+512 < 0x2407D000
    return rows


def writable_inventory(rows):
    result = {}
    for row in rows:
        if row['section'] in 'BbDd' and row['size']:
            name = re.sub(r'\.\d+$', '', row['name'])
            result.setdefault(name, []).append(row)
    return result


def stack(root, directory, elf):
    directory.mkdir()
    includes = ('Core/Inc', 'Drivers/STM32H7xx_HAL_Driver/Inc',
                'Drivers/STM32H7xx_HAL_Driver/Inc/Legacy', 'Drivers/CMSIS/Device/ST/STM32H7xx/Include',
                'Drivers/CMSIS/Include', 'App', 'BSP', 'BSP/lcd')
    flags = ['-mcpu=cortex-m7', '-mthumb', '-mfloat-abi=hard', '-mfpu=fpv5-d16', '-O2', '-g',
             '-ffunction-sections', '-fdata-sections', '-std=c99', '-Wall',
             '-DUSE_HAL_DRIVER', '-DSTM32H743xx', '-fstack-usage']
    sources = sorted((root/'App').rglob('*.c')) + sorted((root/'BSP').glob('*.c'))
    sources += sorted((root/'BSP/lcd').glob('*.c')) + sorted((root/'Core/Src').glob('*.c'))
    sources += sorted((root/'Drivers/STM32H7xx_HAL_Driver/Src').glob('*.c'))
    frames = {}
    frame_rows = []
    for source in sources:
        relative = source.relative_to(root).as_posix()
        target = directory/('_'.join(source.relative_to(root).parts)+'.o')
        output = command([ARM/'arm-none-eabi-gcc.exe', *flags,
                          *[arg for inc in includes for arg in ('-I', root/inc)],
                          '-c', source, '-o', target], target.with_suffix('.log'))
        assert not re.search(r'warning:|error:', output), relative
        for line in target.with_suffix('.su').read_text(encoding='utf-8').splitlines():
            location, size, kind = line.split('\t')
            name = location.rsplit(':', 1)[1]
            frame_rows.append(dict(path=relative, function=name, bytes=int(size), kind=kind))
            frames[name] = max(frames.get(name, 0), int(size))
    return analyze_stack(directory, elf, frame_rows, len(sources))


def analyze_stack(directory, elf, frame_rows, sources_compiled):
    """Reinspect linked calls without recompiling the exact recorded source frames."""
    frames = {}
    for row in frame_rows:
        for name in (row['function'], row['function'].split('.')[0]):
            frames[name] = max(frames.get(name, 0), row['bytes'])
    assembly = command([ARM/'arm-none-eabi-objdump.exe', '-d', elf])
    (directory/'linked-disassembly.log').write_text(assembly, encoding='utf-8')
    graph = {}
    indirect = set()
    current = None
    for line in assembly.splitlines():
        match = re.match(r'^[0-9a-f]+ <([^>]+)>:', line)
        if match:
            current = match[1]
            graph[current] = set()
        elif current:
            call = re.search(r'\s(?:bl|blx|b\.w|b\.n)\s+[0-9a-f]+ <([^>]+)>', line)
            if call and '+' not in call[1] and call[1] != current:
                graph[current].add(call[1])
            if re.search(r'\s(?:blx|bx)\s+(?:r(?:[0-9]|1[0-3])|ip)\b', line):
                indirect.add(current)
    callback_edges = {}
    for caller in indirect:
        targets = []
        if caller.startswith('SCB_'):
            targets = (['CommandDispatch_HandleCommand', 'CommandDispatch_ResetSession']
                       if any(name.startswith('CommandDispatch_HandleCommand') for name in graph)
                       else ['Tuning_HandleCommand', 'Tuning_ResetSession'])
        elif caller.startswith(('HAL_DMA_IRQHandler', 'HAL_DMA_Abort_IT')):
            targets = [name for name in frames if name.startswith(('SPI_DMA', 'ADC_DMA'))]
        elif caller.startswith('HAL_SPI_IRQHandler'):
            targets = [name for name in frames if name.startswith('SPI_') and 'ISR' in name]
        elif caller.startswith('HAL_UART_IRQHandler'):
            targets = [name for name in frames if name.startswith('UART_') and 'ISR' in name]
        elif caller.startswith('ADC_DMAConvCplt'):
            targets = ['ADC_DMAError']
        elif caller.startswith('BSP_LCDIO_Init'):
            targets = ['ST7735_Init', 'BSP_LCDIO_InitBus']
        elif caller.startswith('BSP_LCD_Init'):
            targets = ['ST7735_Init', 'ST7735_FillRect', 'BSP_LCDIO_InitBus']
        elif caller.startswith('BSP_LCD_Clear'):
            targets = ['ST7735_FillRect']
        elif caller.startswith('BSP_LCD_ShowChar'):
            targets = ['ST7735_FillRGBRect']
        elif caller.startswith('ST7735_'):
            # Concrete bus table in BSP/lcd/lcd.c; union is conservative across
            # Init/Read/Write/Send/GetTick uses and includes tail-call wrappers.
            targets = ['BSP_LCDIO_InitBus', 'BSP_LCDIO_WriteRegister',
                       'BSP_LCDIO_ReadRegister', 'BSP_LCDIO_SendData',
                       'BSP_LCDIO_ReceiveData', 'BSP_LCDIO_GetTick']
        elif caller.startswith('st7735_'):
            targets = ['ST7735_ReadRegWrap', 'ST7735_WriteRegWrap',
                       'ST7735_SendDataWrap', 'ST7735_RecvDataWrap']
        actual = sorted({name for target in targets for name in graph
                         if name == target or name.startswith(target+'.')})
        if actual:
            graph[caller].update(actual)
            callback_edges[caller] = actual
    cycles = set()
    memo = {}

    def longest(name, visiting=()):
        if name in visiting:
            cycles.add(name)
            return 0, []
        if name in memo:
            return memo[name]
        options = [longest(child, (*visiting, name)) for child in graph.get(name, ())]
        count, path = max(options, key=lambda row: row[0], default=(0, []))
        result = frames.get(name, 0)+count, [name, *path]
        memo[name] = result
        return result

    roots = ('main', 'TIM15_IRQHandler', 'TIM6_DAC_IRQHandler', 'TIM7_IRQHandler',
             'DMA1_Stream0_IRQHandler', 'DMA1_Stream1_IRQHandler', 'DMA1_Stream2_IRQHandler',
             'SPI2_IRQHandler', 'USART1_IRQHandler', 'EXTI15_10_IRQHandler', 'SysTick_Handler')
    paths = {name: dict(zip(('source_bytes', 'path'), longest(name))) for name in roots}
    groups = ((roots[1], roots[2], roots[10]), (roots[3],), (roots[4],), roots[5:9], (roots[9],))
    source_sum = paths['main']['source_bytes']+sum(max(paths[n]['source_bytes'] for n in group) for group in groups)
    budget = source_sum+5*112+2048
    assert budget <= 8192, ('stack planning budget exceeded', budget)
    return dict(sources_compiled=sources_compiled, all_frames=frame_rows, paths=paths,
                callback_edges=callback_edges, indirect_call_sites=sorted(indirect),
                unresolved_indirect_sites=sorted(indirect-set(callback_edges)), cycles=sorted(cycles),
                non_static_frames=[row for row in frame_rows if row['kind'] != 'static'],
                source_sum_across_priorities=source_sum, hardware_context_reserve=560,
                library_and_unresolved_reserve=2048, budget_bytes=budget, available_stack_bytes=8192,
                limitations='Source frames plus callback unions and explicit reserve, not measured/formal bound; '
                            'SysTick is included in an existing maximum group for comparison with Phase16, '
                            'its lower-priority nesting and unresolved/library work remain in the 2048B reserve.')


def measure(root, directory, with_stack):
    config = root/'App/app_config.h'
    original = config.read_bytes()
    products = [root/'MDK-GCC'/name for name in ('SmartCar.elf', 'SmartCar.hex', 'SmartCar.bin')]
    products.append(root/'MDK-GCC/build/SmartCar.map')
    backup = {path: path.read_bytes() if path.exists() else None for path in products}
    record = dict(builds={})
    directory.mkdir()
    try:
        for lcd in (0, 1):
            configure(root, original, lcd)
            log = directory/f'lcd{lcd}.log'
            output = command([BASH, root/'MDK-GCC/build.sh'], log)
            assert not re.search(r'warning:|error:', output), log
            elf = directory/f'lcd{lcd}.elf'
            shutil.copyfile(products[0], elf)
            map_path = directory/f'lcd{lcd}.map'
            shutil.copyfile(products[-1], map_path)
            size = command([ARM/'arm-none-eabi-size.exe', elf]).splitlines()[1].split()
            record['builds'][str(lcd)] = dict(zip(('text', 'data', 'bss'), map(int, size[:3])))
            record['builds'][str(lcd)].update(warnings=0, errors=0, elf_sha256=sha(elf.read_bytes()),
                                            map_sha256=sha(map_path.read_bytes()), log_sha256=sha(log.read_bytes()))
            print('PASS', directory.name, 'LCD', lcd, *size[:3], flush=True)
        record['symbols'] = symbols(products[0])
        record['writable_objects_by_base_name'] = writable_inventory(record['symbols'])
        record['large_writable_objects'] = [row for row in record['symbols']
                                            if row['section'] in 'BbDd' and row['size'] >= 256]
        if with_stack:
            record['stack'] = stack(root, directory/'stack', products[0])
            print('PASS', directory.name, 'stack planning', record['stack']['budget_bytes'], flush=True)
    finally:
        restore_files({config: original, **backup})
    record['source_sha256'] = {p.relative_to(root).as_posix(): sha(p.read_bytes())
                               for parent in ('App', 'BSP', 'Core') for p in sorted((root/parent).rglob('*'))
                               if p.suffix in ('.c', '.h')}
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'MDK-GCC/build/phase17-resources')
    parser.add_argument('--skip-stack', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve()
    assert not output.exists(), 'Use a fresh output directory; historical evidence is never overwritten'
    output.mkdir(parents=True)
    archive = subprocess.check_output(['git', 'archive', BASE, 'SmartCar'], cwd=REPO)
    baseline = output/'baseline-source'
    baseline.mkdir()
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        tar.extractall(baseline, filter='data')
    record = dict(executed_at=datetime.datetime.now(datetime.timezone.utc).isoformat(), baseline=BASE,
                  current_head=command(['git', 'rev-parse', 'HEAD']).strip(),
                  working_tree_source_paths=command(['git', 'diff', '--name-only', 'HEAD', '--',
                                                     'SmartCar/App', 'SmartCar/BSP', 'SmartCar/Core']).splitlines(),
                  toolchain=command([ARM/'arm-none-eabi-gcc.exe', '--version']).splitlines()[0],
                  ac5_executed=False, hardware_executed=False)
    record['before'] = measure(baseline/'SmartCar', output/'before', not args.skip_stack)
    record['after'] = measure(ROOT, output/'after', not args.skip_stack)
    large = lambda side: Counter((re.sub(r'\.\d+$', '', row['name']), row['size'])
                                 for row in record[side]['large_writable_objects'])
    assert large('before') == large('after'), 'large buffer count or size changed'
    record['large_writable_object_counts_and_sizes_preserved'] = True
    record['delta'] = {str(lcd): {key: record['after']['builds'][str(lcd)][key]-record['before']['builds'][str(lcd)][key]
                                 for key in ('text', 'data', 'bss')} for lcd in (0, 1)}
    destination = ROOT/'tests/results/phase17-resources.json'
    destination.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print('Evidence:', destination)


if __name__ == '__main__':
    main()
