from pathlib import Path
import subprocess,io,tarfile,json
R=Path(__file__).resolve().parents[2];B=R/'MDK-GCC/build/phase17-g4-stack';B.mkdir(exist_ok=True)
old=B/'baseline-source'
if not old.exists():
    old.mkdir()
    data=subprocess.check_output(['git','archive','6125495','SmartCar'],cwd=R.parent)
    with tarfile.open(fileobj=io.BytesIO(data)) as archive:archive.extractall(old,filter='data')
gcc=Path('C:/Users/Li/.eide/tools/gcc_arm/bin/arm-none-eabi-gcc.exe')
includes=['Core/Inc','Drivers/STM32H7xx_HAL_Driver/Inc','Drivers/STM32H7xx_HAL_Driver/Inc/Legacy',
          'Drivers/CMSIS/Device/ST/STM32H7xx/Include','Drivers/CMSIS/Include','App','BSP','BSP/lcd']
common=['system/lifecycle','system/scheduler','system/safety','control/tracking']
before=['services/param','control/tuning','hmi/menu']+common
after=['services/parameter_bank','services/parameter_store','system/parameter_application',
       'control/tuning','control/tuning_values','system/tuning_application','services/command_dispatch',
       'hmi/menu','hmi/menu_keys','hmi/menu_render']+common
record={}
for label,root,modules in [('before',old/'SmartCar',before),('after',R,after)]:
    directory=B/label;directory.mkdir(exist_ok=True);frames={}
    for module in modules:
        target=directory/(module.replace('/','_')+'.o')
        args=[str(gcc),'-mcpu=cortex-m7','-mthumb','-mfloat-abi=hard','-mfpu=fpv5-d16','-O2','-g',
              '-ffunction-sections','-fdata-sections','-std=c99','-Wall','-DUSE_HAL_DRIVER',
              '-DSTM32H743xx','-fstack-usage']
        args += [arg for inc in includes for arg in ['-I',str(root/inc)]]
        args += ['-c',str(root/'App'/(module+'.c')),'-o',str(target)]
        result=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        target.with_suffix('.log').write_bytes(result.stdout)
        assert result.returncode==0,(module,result.stdout.decode(errors='replace'))
        for line in target.with_suffix('.su').read_text().splitlines():
            name,size,kind=line.split('\t');frames[name.rsplit(':',1)[1]]={'bytes':int(size),'kind':kind}
    record[label]=frames
    print(label,'functions',len(frames),'maximum frame',max(v['bytes'] for v in frames.values()),flush=True)
(B/'frames.json').write_text(json.dumps(record,indent=2),encoding='utf-8')
for label,names in [('before',['Tuning_Service','Tuning_Set','Tuning_QueryParams','Tracking_Observe','Scheduler_ControlISR']),
                    ('after',['CommandDispatch_Service','CommandDispatch_Set','TuningApplication_Set',
                              'CommandDispatch_QueryParams','Tracking_Observe','Scheduler_ControlISR'])]:
    print(label,{n:record[label][n] for n in names if n in record[label]})
