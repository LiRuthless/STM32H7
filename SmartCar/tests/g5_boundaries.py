"""Exercise actual split owners and compare unchanged codec bodies to G4."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from app_fixture import APP, flatten_includes
from control_equivalence import current_names
import datalog_v2 as logger
import io
import tarfile

ROOT = Path(__file__).resolve().parents[1]
BASE = '7eefb95'
TOKEN = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\w+|[^\s]', re.S)


def bodies(text):
    items = [m[0] for m in TOKEN.finditer(text) if not m[0].startswith(('/*', '//'))]
    result = {}
    depth = 0
    for i, token in enumerate(items):
        if token == '{' and not depth and items[i-1] == ')':
            j = i - 2
            level = 1
            while level:
                level += (items[j] == ')') - (items[j] == '(')
                j -= 1
            name = items[j]
            end = i + 1
            level = 1
            while level:
                level += (items[end] == '{') - (items[end] == '}')
                end += 1
            assert name not in result
            result[name] = items[i:end]
        depth += (token == '{') - (token == '}')
    return result


def structural():
    previous = subprocess.check_output(['git', 'show', BASE+':SmartCar/App/services/scb_protocol.c'],
                                       cwd=ROOT.parent).decode('utf-8')
    old = bodies(previous)
    moved = {}
    for name in ('scb_frame', 'scb_json', 'scb_command_codec'):
        current = bodies((APP/(name+'.c')).read_text(encoding='utf-8'))
        for function, body in current.items():
            assert function not in moved, function
            moved[function] = body
    for function, expected in old.items():
        if function != 'SCB_CalculateCrc':
            assert moved[function] == expected, function
    checksum = bodies((APP/'checksum.c').read_text(encoding='utf-8'))['Checksum_UpdateCrc32']
    assert checksum == old['SCB_CalculateCrc']
    old_link = subprocess.check_output(['git', 'show', BASE+':SmartCar/App/services/scb_link.c'],
                                      cwd=ROOT.parent).decode('utf-8')
    assert bodies(old_link) == bodies((APP/'scb_link.c').read_text(encoding='utf-8'))
    for module in ('run_record', 'logger', 'log_store', 'log_export'):
        source = (APP/(module+'.c')).read_text(encoding='utf-8')
        real = ' '.join(m[0] for m in TOKEN.finditer(source) if not m[0].startswith(('/*', '//')))
        assert not re.search(r'\b(?:SCB_|scb_)[A-Za-z0-9_]*', real), module
        if module != 'log_store':
            assert not re.search(r'\bBSP_Flash_Log\w+', real), module
    assert not (ROOT/'App/services/scb_protocol.c').exists()
    assert not (ROOT/'App/services/scb_protocol.h').exists()
    assert not (ROOT/'App/services/datalog.c').exists()
    print('PASS G5 structural: 27 codec bodies and CRC formula match G4; link bodies unchanged; unique Flash backend')


PREFIX = logger.HARNESS.split('int main(void)')[0]
PREFIX = PREFIX.replace('#include <string.h>', r'''#include <string.h>
static unsigned compares;
static int G5_Memcmp(const void *a,const void *b,size_t n);
#define memcmp G5_Memcmp''')
PREFIX = PREFIX.replace('static uint8_t flash[8388608];', r'''#undef memcmp
static int G5_Memcmp(const void *a,const void *b,size_t n){++compares;return memcmp(a,b,n);}
static uint8_t flash[8388608];''')
PREFIX = PREFIX.replace('uint32_t BSP_Time_GetTick(void){return 0;}',
                        'static uint32_t now;\nuint32_t BSP_Time_GetTick(void){return now;}')
PREFIX = PREFIX.replace('static int unavailable,write_fail,read_fail,corrupt,inject;', r'''
static int unavailable,write_fail,read_fail,corrupt,inject,partial_read;
static unsigned reads,feeds,order_length;
static char order[32];''')
PREFIX = PREFIX.replace('void BSP_WDT_Feed(void){}', 'void BSP_WDT_Feed(void){++feeds;}')
PREFIX = PREFIX.replace('assert(a+n<=8388608);if(read_fail',
                        "assert(a+n<=8388608);++reads;if(order_length<sizeof order)order[order_length++]='R';"
                        'if(partial_read){b[0]=0x55;return 1;}if(read_fail')
PREFIX = PREFIX.replace('++page_calls;', "++page_calls;if(order_length<sizeof order)order[order_length++]='W';")

MAIN = r'''
#include "scb_command_codec.h"
int main(void)
{
    datalog_export_status_t info;
    const uint8_t marker=0;
    const uint8_t *raw=&marker;
    const uint8_t *first;
    uint16_t length=999;
    uint8_t guarded[970];
    unsigned before;
    Gray_Init();start();
    /* Verify the actual storage wrapper's write/read/compare short circuit. */
    write_fail=1;order_length=compares=0;
    assert(LogStore_WriteHeader(RunRecord_GetHeader())==1);
    assert(order_length==1&&order[0]=='W'&&!compares);
    write_fail=0;read_fail=1;order_length=compares=0;
    assert(LogStore_WriteHeader(RunRecord_GetHeader())==1);
    assert(order_length==2&&!memcmp(order,"WR",2)&&!compares);
    read_fail=0;corrupt=1;order_length=compares=0;
    assert(LogStore_WriteHeader(RunRecord_GetHeader())==1);
    assert(order_length==2&&!memcmp(order,"WR",2)&&compares==1);
    corrupt=0;order_length=compares=0;
    assert(LogStore_WriteHeader(RunRecord_GetHeader())==0);
    assert(order_length==2&&!memcmp(order,"WR",2)&&compares==1);
    for(unsigned i=0;i<4;++i)Datalog_Push();
    Datalog_Stop();assert(Datalog_GetCount()==4);
    now=123;assert(Datalog_BeginExport(&info)==DATALOG_RESULT_OK);
    memset(s_export_block,0xa5,8);
    before=reads;unsigned previous_feeds=feeds;
    assert(Datalog_ReadExport(info.transfer_id,0,32,&raw,&length)==DATALOG_RESULT_OK);
    assert(raw==s_export_block+8&&length==32&&reads==before+1&&feeds==previous_feeds+2);
    assert(!memcmp(raw,flash,32));first=raw;
    for(unsigned i=0;i<8;++i)assert(s_export_block[i]==0xa5);
    uint32_t progress=s_export_progress_ms,crc=s_export.crc32,next=s_export.next_offset;
    now=progress+9999;before=reads;previous_feeds=feeds;
    raw=&marker;length=999;
    assert(Datalog_ReadExport(info.transfer_id,0,32,&raw,&length)==DATALOG_RESULT_OK);
    assert(raw==first&&length==32&&reads==before&&feeds==previous_feeds);
    assert(s_export_progress_ms==progress&&s_export.crc32==crc&&s_export.next_offset==next);
    raw=&marker;length=999;
    assert(Datalog_ReadExport(info.transfer_id,32,961,&raw,&length)==DATALOG_RESULT_OUT_OF_RANGE);
    assert(raw==&marker&&!length);
    raw=&marker;length=999;
    assert(Datalog_ReadExport(info.transfer_id+1,32,1,&raw,&length)==DATALOG_RESULT_NOT_FOUND);
    assert(raw==&marker&&!length);
    raw=&marker;length=999;
    assert(Datalog_ReadExport(info.transfer_id,33,1,&raw,&length)==DATALOG_RESULT_INVALID_ARG);
    assert(raw==&marker&&!length);
    /* New read reuses the same raw storage; first eight reserved bytes stay untouched. */
    assert(Datalog_ReadExport(info.transfer_id,32,50,&raw,&length)==DATALOG_RESULT_OK);
    assert(raw==first&&length==50&&!memcmp(raw,flash+32,50));
    crc=s_export.crc32;next=s_export.next_offset;progress=s_export_progress_ms;
    Logger_LatchFault(DATALOG_FAULT_WRITE);
    partial_read=1;raw=&marker;length=999;previous_feeds=feeds;
    assert(Datalog_ReadExport(info.transfer_id,82,1,&raw,&length)==DATALOG_RESULT_IO_ERROR);
    assert(raw==&marker&&!length&&!s_export.active&&!s_export_last_length);
    assert(s_status.fault==DATALOG_FAULT_WRITE&&feeds==previous_feeds+2);
    assert(s_export.crc32==crc&&s_export.next_offset==next&&s_export_progress_ms==progress);
    assert(s_export_block[8]==0x55);
    for(unsigned i=0;i<8;++i)assert(s_export_block[i]==0xa5);
    partial_read=0;
    /* Real codec writes exactly 968 bytes to its caller-owned target. */
    memset(guarded,0xcc,sizeof guarded);
    assert(SCB_EncodeLogData(guarded+1,0x78563412u,flash,960)==968);
    const uint8_t head[8]={0x12,0x34,0x56,0x78,0xc0,0x03,0,0};
    assert(guarded[0]==0xcc&&guarded[969]==0xcc&&!memcmp(guarded+1,head,8));
    assert(!memcmp(guarded+9,flash,960));
    now=0xfffffff0u;assert(Datalog_BeginExport(&info)==DATALOG_RESULT_OK);
    raw=&marker;length=999;now=0xfffffff0u+10000u;
    assert(Datalog_ReadExport(info.transfer_id,0,1,&raw,&length)==DATALOG_RESULT_NOT_FOUND);
    assert(raw==&marker&&!length&&!Datalog_IsExporting());
    puts("PASS G5 real owners: raw lifetime/failure outputs/repeat lease/partial read/first fault, codec 968B guards, store WR short circuit");
}
'''


def main():
    structural()
    with tempfile.TemporaryDirectory(prefix='g5_boundaries_') as temporary:
        work=Path(temporary)
        for name in ('bsp_flash.c','bsp_flash.h','bsp_gray.h','bsp_adc.h'):
            shutil.copyfile(ROOT/'BSP'/name,work/name)
        for name in ('gray_sensor.c','gray_sensor.h','run_record.c','logger.c','log_store.c','log_export.c','telemetry.c'):
            shutil.copyfile(APP/name,work/name)
        (work/'main.h').write_text('#include <stdint.h>\n',encoding='utf-8')
        (work/'bsp_w25q64.h').write_text(logger.W25,encoding='utf-8')
        (work/'bsp.h').write_text(logger.BSP,encoding='utf-8')
        black=','.join('1000' if i&1 else '0' for i in range(16))
        white=','.join('0' if i&1 else '1000' for i in range(16))
        (work/'gray_calibration.h').write_text(
            f'static const int16_t s_gray_black[16]={{{black}}};\n'
            f'static const int16_t s_gray_white[16]={{{white}}};\n',encoding='utf-8')
        (work/'test.c').write_text(current_names(PREFIX+MAIN),encoding='utf-8')
        binary=work/'test.exe'
        sources=[work/'test.c',work/'gray_sensor.c',work/'bsp_flash.c',work/'telemetry.c']
        sources += [APP/(name+'.c') for name in ('checksum','scb_frame','scb_json','scb_command_codec','tuning_values')]
        subprocess.run(['gcc','-std=c99','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function',
                        '-I',str(work),'-I',str(APP),*map(str,sources),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
    historical_records()


def historical_records():
    """Compare actual G4 and split C output, without deriving expected bytes."""
    replay=r'''
int main(int argc,char **argv)
{
    assert(argc==2);Gray_Init();start();
    for(unsigned i=0;i<11;++i)
    {
        tracking.control_sequence=0xfffffff8u+i;
        tracking.active_error=(int16_t)(i*31-150);
        tracking.track_out=(int16_t)(500-i*73);
        battery_filt=(int16_t)(1300+i);
        dl1b_distance_mm=(uint16_t)(8000+i);
        for(unsigned j=0;j<4;++j)track.filtered[j]=(int16_t)(100+i*4+j);
        track.error=(int16_t)(i*10);em.kernel_state=i%7;em.roundabout_state=i%4;
        Gray_Update();Datalog_PublishControl();Datalog_Push();
        if(i==7){inject=1;Datalog_Service();}
    }
    Datalog_Stop();assert(s_status.written==12);
    FILE *file=fopen(argv[1],"wb");assert(file);
    assert(fwrite(flash,1,256+s_status.written*96,file)==256+s_status.written*96);fclose(file);
    Datalog_Dump();fputs(output,stdout);
}
'''
    with tempfile.TemporaryDirectory(prefix='g5_bytes_') as temporary:
        work=Path(temporary)
        archived=work/'archive';archived.mkdir()
        raw=subprocess.check_output(['git','archive',BASE,'SmartCar/App'],cwd=ROOT.parent)
        with tarfile.open(fileobj=io.BytesIO(raw)) as archive:
            archive.extractall(archived,filter='data')
        baseline=work/'baseline';baseline.mkdir()
        for path in (archived/'SmartCar/App').rglob('*'):
            if path.suffix in ('.c','.h'):
                (baseline/path.name).write_bytes(flatten_includes(path.read_bytes()))
        outcomes=[]
        for label,source,modules in [('before',baseline,['datalog','scb_protocol']),
                                     ('after',APP,['run_record','logger','log_store','log_export',
                                                   'checksum','scb_frame','scb_json','scb_command_codec'])]:
            directory=work/label;directory.mkdir()
            for name in ('bsp_flash.c','bsp_flash.h','bsp_gray.h','bsp_adc.h'):
                shutil.copyfile(ROOT/'BSP'/name,directory/name)
            for name in ('gray_sensor.c','gray_sensor.h','telemetry.c'):
                shutil.copyfile(source/name,directory/name)
            for module in modules[:1] if label=='before' else modules[:4]:
                shutil.copyfile(source/(module+'.c'),directory/(module+'.c'))
            (directory/'main.h').write_text('#include <stdint.h>\n',encoding='utf-8')
            (directory/'bsp_w25q64.h').write_text(logger.W25,encoding='utf-8')
            (directory/'bsp.h').write_text(logger.BSP,encoding='utf-8')
            (directory/'gray_calibration.h').write_text(
                'static const int16_t s_gray_black[16]={0};\n'
                'static const int16_t s_gray_white[16]={'+','.join(['1000']*16)+'};\n',encoding='utf-8')
            prefix=logger.HARNESS.split('int main(void)')[0]
            if label=='before':
                includes='\n'.join('#include "'+module+'.c"' for module in
                                   ('run_record','logger','log_store','log_export'))
                assert includes in prefix
                prefix=prefix.replace(includes,'#include "datalog.c"')
            (directory/'test.c').write_text(current_names(prefix+replay),encoding='utf-8')
            binary=directory/'test.exe';output=directory/'output.bin'
            codec=['scb_protocol'] if label=='before' else ['checksum','scb_frame','scb_json','scb_command_codec']
            sources=[directory/'test.c',directory/'gray_sensor.c',directory/'bsp_flash.c',directory/'telemetry.c']
            sources += [source/(module+'.c') for module in codec+['tuning_values']]
            subprocess.run(['gcc','-std=c99','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function',
                            '-I',str(directory),'-I',str(source),*map(str,sources),'-o',str(binary)],check=True)
            run=subprocess.run([str(binary),str(output)],capture_output=True,check=True)
            outcomes.append((run.stdout,output.read_bytes()))
        assert outcomes[0]==outcomes[1], 'Actual G4/current CSV or header/record bytes differ'
    print('PASS G5 actual G4/current: 256B header, twelve 96B records and complete CSV bytes identical under ISR injection')


if __name__=='__main__':
    main()
