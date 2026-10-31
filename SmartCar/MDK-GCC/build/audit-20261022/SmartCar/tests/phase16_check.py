"""Repeat Phase 16 native gates, dual LCD builds and GCC resource evidence.

Run with the repository's configured Python and native GCC on PATH:
    python SmartCar/tests/phase16_check.py --build --budget
Raw products stay in MDK-GCC/build; no AC5/hardware result is inferred.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parent
BUILD = ROOT / 'MDK-GCC/build'
ARM = Path('C:/Users/Li/.eide/tools/gcc_arm/bin')
GCC = ARM / 'arm-none-eabi-gcc.exe'
BASH = Path('C:/Program Files/Git/bin/bash.exe')
TESTS = ('scb_protocol', 'scb_link', 'spi_wifi_state', 'control_equivalence',
         'dual_mode_tracking', 'datalog_v2', 'runtime_tuning', 'spi_log_export')


def command(args, log=None):
    result = subprocess.run(list(map(str, args)), cwd=REPO, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    if log:
        log.write_bytes(result.stdout)
    if result.returncode:
        print(result.stdout.decode('utf-8', errors='replace'))
    result.check_returncode()
    return result.stdout.decode('utf-8', errors='replace')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def archive_artifacts():
    directory = BUILD / 'phase16-artifacts'
    directory.mkdir(exist_ok=True)
    result = {}
    for source in (ROOT/'MDK-GCC/SmartCar.elf', BUILD/'SmartCar.map'):
        destination = directory/source.name
        shutil.copyfile(source, destination)
        assert digest(destination) == digest(source)
        result[str(destination.relative_to(ROOT))] = digest(destination)
    return result


def configuration(lcd, original):
    data, count = re.subn(rb'(#define LCD_TARGET_EXTERNAL )0\b',
                         lambda m: m[1] + str(lcd).encode(), original)
    assert count == 1, 'source LCD configuration must be restored to 0'
    (ROOT / 'App/app_config.h').write_bytes(data)


def static_checks():
    ioc = (ROOT / 'SmartCar.ioc').read_text(encoding='utf-8')
    for irq, priority in [('DMA1_Stream0', 2), ('DMA1_Stream1', 3), ('DMA1_Stream2', 3),
                          ('SPI2', 3), ('EXTI15_10', 4), ('TIM6_DAC', 0), ('TIM7', 1)]:
        assert f'NVIC.{irq}_IRQn=true\\:{priority}\\:0' in ioc
    for peripheral, stream in [('ADC1', 0), ('SPI2_TX', 1), ('SPI2_RX', 2)]:
        assert f'Dma.{peripheral}.0.Instance=DMA1_Stream{stream}' in ioc
    assert 'RCC.HSE_VALUE=25000000' in ioc
    assert 'RCC.SPI123Freq_Value=480000000' in ioc
    assert 'SPI2.BaudRatePrescaler=SPI_BAUDRATEPRESCALER_256' in ioc
    main = (ROOT / 'Core/Src/main.c').read_text(encoding='utf-8')
    for key, value in [('PLLM', 5), ('PLLN', 192), ('PLLQ', 2)]:
        assert f'RCC_OscInitStruct.PLL.{key} = {value};' in main
    assert 'RCC_SPI123CLKSOURCE_PLL' in (ROOT / 'Core/Src/spi.c').read_text(encoding='utf-8')
    assert 'periph_clk_init.PLL3.PLL3Q = 4;' in main
    assert 'RCC_SPI45CLKSOURCE_PLL3' in (ROOT / 'App/app.c').read_text(encoding='utf-8')
    sampler = (ROOT / 'BSP/bsp_sampler.c').read_text(encoding='utf-8')
    assert 'HAL_NVIC_SetPriority(TIM15_IRQn, 0, 0)' in sampler
    assert 'HAL_NVIC_SetPriority(TIM7_IRQn, 1, 0)' in (ROOT / 'Core/Src/tim.c').read_text(encoding='utf-8')
    startup = (ROOT / 'MDK-ARM/startup_stm32h743xx.s').read_text(encoding='utf-8')
    linker = (ROOT / 'MDK-GCC/STM32H743VITx.ld').read_text(encoding='utf-8')
    assert re.search(r'Stack_Size\s+EQU\s+0x2000', startup)
    assert re.search(r'_Min_Stack_Size\s*=\s*0x2000', linker)
    project = (ROOT / 'MDK-ARM/SmartCar.uvprojx').read_text(encoding='utf-8')
    for name in ('scb_protocol', 'scb_link', 'telemetry', 'tuning', 'bsp_wifi'):
        assert f'<FileName>{name}.c</FileName>' in project
    calibration = (ROOT / 'App/gray_calibration.h').read_text(encoding='utf-8')
    calibration = re.sub(r'/\*.*?\*/', '', calibration, flags=re.S)
    for name in ('s_gray_black', 's_gray_white'):
        body = re.search(name+r'\[16\]\s*=\s*\{([^}]+)\}', calibration).group(1)
        assert [int(value) for value in re.findall(r'-?\d+', body)] == [-1]*16
    sources = ['App/app.c', 'App/scb_protocol.c', 'App/scb_link.c', 'App/tuning.c',
               'App/telemetry.c', 'App/datalog.c', 'BSP/bsp_wifi.c', 'SmartCar.ioc',
               'MDK-GCC/STM32H743VITx.ld', 'MDK-ARM/startup_stm32h743xx.s']
    return {'priorities_verified': True, 'dma_streams_verified': True,
            'spi2_kernel_hz': 480000000, 'startup_prescaler': 256,
            'startup_sck_hz': 1875000, 'fast_prescaler': 64, 'fast_sck_hz': 7500000,
            'stack_reserved_bytes': 8192, 'calibration_still_invalid': True,
            'source_sha256': {name: digest(ROOT / name) for name in sources}}


def symbol_evidence():
    output = command([ARM / 'arm-none-eabi-nm.exe', '-S', '-n', ROOT / 'MDK-GCC/SmartCar.elf'])
    symbols = {}
    for line in output.splitlines():
        parts = line.split()
        if len(parts) == 4:
            symbols[parts[3]] = {'address': int(parts[0], 16), 'size': int(parts[1], 16)}
        elif len(parts) == 3:
            symbols[parts[2]] = {'address': int(parts[0], 16), 'size': 0}
    assert symbols['s_tx'] == {'address': 0x2407F000, 'size': 1024}
    assert symbols['s_rx'] == {'address': 0x2407F400, 'size': 1024}
    assert symbols['_estack']['address'] == 0x2407F000
    assert symbols['_ebss']['address'] + 512 < 0x2407D000
    assert symbols['s_adc_buf'] == {'address': 0x30000000, 'size': 20}
    selected = ('s_tx', 's_rx', 's_adc_buf', 's_requests', 's_slots', 's_buf', 's_events',
                's_export_block', 's_query_json', 's_command_json', '_ebss', '_estack', 'end')
    return {name: symbols[name] for name in selected if name in symbols}


def path_budget(frames, directory):
    """Combine actual linked calls and source frames; keep the library reserve explicit."""
    assembly = command([ARM/'arm-none-eabi-objdump.exe', '-d', ROOT/'MDK-GCC/SmartCar.elf'])
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
            if re.search(r'\sblx\s+r\d+', line):
                indirect.add(current)
    # Conservative callback union: streams share a HAL dispatcher but configured owners stay separate.
    callbacks = {
        'SCB_ReceiveFrame': ['Tuning_HandleCommand'],
        'SCB_ResetSession': ['Tuning_ResetSession'],
        'HAL_DMA_IRQHandler': [name for name in frames if name.startswith(('SPI_DMA', 'ADC_DMA'))],
        'HAL_SPI_IRQHandler': [name for name in frames if name.startswith('SPI_') and 'ISR' in name],
        'HAL_UART_IRQHandler': [name for name in frames if name.startswith('UART_') and 'ISR' in name],
    }
    for caller, targets in callbacks.items():
        if caller in graph:
            graph[caller].update(target for target in targets if target in graph)
    memo = {}
    cycles = set()

    def longest(name, visiting=()):
        if name in visiting:
            cycles.add(name)
            return 0, []
        if name in memo:
            return memo[name]
        children = [longest(child, (*visiting, name)) for child in graph.get(name, ())]
        child_size, path = max(children, key=lambda pair: pair[0], default=(0, []))
        result = (frames.get(name, {}).get('bytes', 0)+child_size, [name, *path])
        memo[name] = result
        return result

    roots = ('main', 'TIM15_IRQHandler', 'TIM6_DAC_IRQHandler', 'TIM7_IRQHandler',
             'DMA1_Stream0_IRQHandler', 'DMA1_Stream1_IRQHandler', 'DMA1_Stream2_IRQHandler',
             'SPI2_IRQHandler', 'USART1_IRQHandler', 'EXTI15_10_IRQHandler', 'SysTick_Handler', 'App_Loop')
    paths = {name: {'source_bytes': longest(name)[0], 'path': longest(name)[1]} for name in roots}
    # Priority 0/1/2/3/4 can nest once each; TIM15/TIM6 cannot preempt each other.
    groups = ((roots[1], roots[2], roots[10]), (roots[3],), (roots[4],), roots[5:9], (roots[9],))
    source_total = paths['main']['source_bytes'] + sum(
        max(paths[name]['source_bytes'] for name in group) for group in groups)
    # 32-byte basic + 72-byte FP frame, use 112 to include alignment for every level.
    hardware_contexts = 5*112
    library_and_unresolved_reserve = 2048
    budget = source_total+hardware_contexts+library_and_unresolved_reserve
    assert budget <= 8192, ('8KB reserve exceeded; stop and clarify', budget)
    return {'paths': paths, 'source_frame_sum_across_priorities': source_total,
            'hardware_context_reserve': hardware_contexts,
            'library_and_unresolved_reserve': library_and_unresolved_reserve,
            'budget_bytes': budget, 'available_stack_bytes': 8192,
            'indirect_call_sites': sorted(indirect), 'cycles_seen': sorted(cycles),
            'limitations': 'Source-frame sum is conservative for tail calls; libraries and unlisted '
                           'indirect callees use a planning reserve, not a formal bound or measurement.'}


def stack_evidence(original):
    """Compile exact source with configured flags. No measured high-water claim."""
    configuration(1, original)
    directory = BUILD / 'stack-final'
    directory.mkdir(exist_ok=True)
    includes = ('Core/Inc', 'Drivers/STM32H7xx_HAL_Driver/Inc',
                'Drivers/STM32H7xx_HAL_Driver/Inc/Legacy',
                'Drivers/CMSIS/Device/ST/STM32H7xx/Include', 'Drivers/CMSIS/Include',
                'App', 'BSP', 'BSP/lcd')
    flags = ['-mcpu=cortex-m7', '-mthumb', '-mfloat-abi=hard', '-mfpu=fpv5-d16',
             '-O2', '-g', '-ffunction-sections', '-fdata-sections', '-std=c99', '-Wall',
             '-DUSE_HAL_DRIVER', '-DSTM32H743xx', '-fstack-usage']
    sources = list((ROOT / 'App').glob('*.c')) + list((ROOT / 'BSP').glob('*.c'))
    sources += list((ROOT / 'BSP/lcd').glob('*.c')) + list((ROOT / 'Core/Src').glob('*.c'))
    sources += list((ROOT / 'Drivers/STM32H7xx_HAL_Driver/Src').glob('*.c'))
    for source in sources:
        target = directory / ('_'.join(source.relative_to(ROOT).parts) + '.o')
        result = command([GCC, *flags, *[item for path in includes for item in ('-I', ROOT/path)],
                          '-c', source, '-o', target], target.with_suffix('.log'))
        assert not re.search(r'warning:|error:', result), source
    frames = {}
    for source in directory.glob('*.su'):
        for line in source.read_text(encoding='utf-8').splitlines():
            location, size, kind = line.split('\t')
            name = location.rsplit(':', 1)[1]
            value = {'bytes': int(size), 'kind': kind}
            if name not in frames or frames[name]['bytes'] < value['bytes']:
                frames[name] = value
    (directory / 'frames.json').write_text(json.dumps(frames, indent=2), encoding='utf-8')
    # Record actual public/private frames for manual path review; never silently discard dynamic frames.
    prefixes = ('App_', 'SCB_', 'Tuning_', 'Telemetry_', 'Datalog_', 'Tracking_',
                'ElectromagneticFSM_', 'OpticalFSM_', 'BSP_WiFi_', 'BSP_Gray_', 'BSP_ADC_')
    selected = {name: value for name, value in frames.items() if name.startswith(prefixes)}
    handlers = ('TIM15_IRQHandler', 'TIM6_DAC_IRQHandler', 'TIM7_IRQHandler',
                'DMA1_Stream0_IRQHandler', 'DMA1_Stream1_IRQHandler', 'DMA1_Stream2_IRQHandler',
                'SPI2_IRQHandler', 'EXTI15_10_IRQHandler', 'HAL_TIM_IRQHandler',
                'HAL_DMA_IRQHandler', 'HAL_SPI_IRQHandler', 'HAL_GPIO_EXTI_IRQHandler')
    selected.update({name: frames[name] for name in handlers if name in frames})
    selected['main'] = frames['main']
    return {'configuration': 'LCD1 -O2 hard-float; compile-only same source',
            'sources_compiled': len(sources), 'frames': selected,
            'path_budget': path_budget(frames, directory),
            'non_static_frames': {name: value for name, value in frames.items()
                                  if value['kind'] != 'static'},
            'limitations': 'GCC source frame sizes; library/indirect call and hardware contexts '
                           'require the path budget in validation.md; no measured watermark.'}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build', action='store_true')
    parser.add_argument('--budget', action='store_true')
    args = parser.parse_args()
    if args.budget and not args.build:
        parser.error('--budget requires --build so the ELF matches the source frames')
    BUILD.mkdir(exist_ok=True)
    temporary = BUILD / 'host-temp'
    temporary.mkdir(exist_ok=True)
    os.environ.update(TEMP=str(temporary), TMP=str(temporary), PYTHONUTF8='1')
    config = ROOT / 'App/app_config.h'
    original = config.read_bytes()
    record = {'executed_at': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'toolchain': command([GCC, '--version']).splitlines()[0],
              'native_gcc': command(['gcc', '--version']).splitlines()[0],
              'tests': {}, 'builds': {}, 'static': static_checks(),
              'ac5_available': shutil.which('armcc') is not None,
              'hardware_executed': False, 'phase_complete': False}
    try:
        for name in TESTS:
            path = BUILD / f'g5-{name}.log'
            output = command([sys.executable, ROOT/'tests'/f'{name}.py'], path)
            record['tests'][name] = {'exit_code': 0, 'log_sha256': digest(path),
                                     'summary': output.strip().splitlines()[-1]}
            print('PASS', name, flush=True)
        if args.build:
            for lcd in (0, 1):
                configuration(lcd, original)
                path = BUILD / f'g5-lcd{lcd}.log'
                output = command([BASH, ROOT/'MDK-GCC/build.sh'], path)
                assert not re.search(r'warning:|error:', output)
                size = command([ARM/'arm-none-eabi-size.exe', ROOT/'MDK-GCC/SmartCar.elf'])
                text, data, bss = map(int, size.splitlines()[1].split()[:3])
                record['builds'][str(lcd)] = {'warnings': 0, 'errors': 0, 'text': text,
                                              'data': data, 'bss': bss,
                                              'log_sha256': digest(path)}
                print('PASS LCD', lcd, text, data, bss, flush=True)
            record['symbols'] = symbol_evidence()
        if args.budget:
            record['stack'] = stack_evidence(original)
            print('PASS stack compilation', record['stack']['sources_compiled'], flush=True)
        if args.build:
            record['artifacts_sha256'] = archive_artifacts()
    finally:
        config.write_bytes(original)
    destination = ROOT / 'tests/results/phase16.json'
    destination.parent.mkdir(exist_ok=True)
    destination.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print('Evidence:', destination)


if __name__ == '__main__':
    main()
