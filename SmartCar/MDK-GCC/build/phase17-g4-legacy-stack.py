from pathlib import Path
exec(Path(__file__).with_name('phase17-g4-stack.py').read_text(encoding='utf-8').split('record={}')[0])
for label,root in [('before',old/'SmartCar'),('after',R)]:
    target=B/(label+'-approved-legacy_control.o')
    args=[str(gcc),'-mcpu=cortex-m7','-mthumb','-mfloat-abi=hard','-mfpu=fpv5-d16','-O2',
          '-ffunction-sections','-fdata-sections','-std=c99','-Wall','-DUSE_HAL_DRIVER',
          '-DSTM32H743xx','-fstack-usage']
    args += [arg for inc in includes for arg in ['-I',str(root/inc)]]
    args += ['-c',str(root/'App/control/legacy_control.c'),'-o',str(target)]
    result=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    assert result.returncode==0 and not result.stdout,result.stdout.decode(errors='replace')
    print(label,target.with_suffix('.su').read_text().strip())
