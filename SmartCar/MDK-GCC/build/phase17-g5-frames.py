from pathlib import Path
import subprocess,io,tarfile,json
R=Path(__file__).resolve().parents[2]
B=R/'MDK-GCC/build/phase17-g5-stack';B.mkdir(exist_ok=True)
old=B/'baseline-source'
if not old.exists():
    old.mkdir()
    data=subprocess.check_output(['git','archive','7eefb95','SmartCar/App'],cwd=R.parent)
    with tarfile.open(fileobj=io.BytesIO(data)) as archive:archive.extractall(old,filter='data')
gcc=Path('C:/Users/Li/.eide/tools/gcc_arm/bin/arm-none-eabi-gcc.exe')
includes=['Core/Inc','Drivers/STM32H7xx_HAL_Driver/Inc','Drivers/STM32H7xx_HAL_Driver/Inc/Legacy',
          'Drivers/CMSIS/Device/ST/STM32H7xx/Include','Drivers/CMSIS/Include','BSP','BSP/lcd']
before=['datalog','scb_protocol','command_dispatch','scb_link']
after=['run_record','logger','log_store','log_export','checksum','scb_frame','scb_json',
       'scb_command_codec','command_dispatch','scb_link']
record={}
for label,root,modules in [('before',old/'SmartCar',before),('after',R,after)]:
    directory=B/label;directory.mkdir(exist_ok=True);frames={}
    for module in modules:
        target=directory/(module+'.o')
        args=[str(gcc),'-mcpu=cortex-m7','-mthumb','-mfloat-abi=hard','-mfpu=fpv5-d16','-O2','-g',
              '-ffunction-sections','-fdata-sections','-std=c99','-Wall','-DUSE_HAL_DRIVER',
              '-DSTM32H743xx','-fstack-usage','-I',str(root/'App')]
        args += [arg for inc in includes for arg in ['-I',str(R/inc)]]
        args += ['-c',str(root/'App/services'/(module+'.c')),'-o',str(target)]
        result=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        target.with_suffix('.log').write_bytes(result.stdout)
        assert result.returncode==0 and not result.stdout,(module,result.stdout.decode(errors='replace'))
        for line in target.with_suffix('.su').read_text().splitlines():
            name,size,kind=line.split('\t');frames[name.rsplit(':',1)[1]]={'bytes':int(size),'kind':kind}
    record[label]=frames
    print(label,'functions',len(frames),'maximum frame',max(v['bytes'] for v in frames.values()),flush=True)
(B/'frames.json').write_text(json.dumps(record,indent=2),encoding='utf-8')
for name in ['Datalog_Start','Datalog_Push','Datalog_Service','Datalog_Dump','Datalog_ReadExport',
             'RunRecord_BuildHeader','RunRecord_CaptureSample','LogStore_WriteHeader',
             'SCB_CalculateCrc','Checksum_UpdateCrc32','SCB_ParseJson','SCB_ParseCommand',
             'CommandDispatch_LogCommand']:
    print(name,record['before'].get(name),record['after'].get(name))
