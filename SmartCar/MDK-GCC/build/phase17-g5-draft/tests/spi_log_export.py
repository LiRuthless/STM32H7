"""Real App/logger/export/command C, independent Python zlib and time-axis checks."""
import json
import struct
import tempfile
import zlib
from pathlib import Path
import dual_mode_tracking as dual
import runtime_tuning as tuning

PREFIX = tuning.prefix.replace('static unsigned parameter_writes, event_counts[8];',
    'static unsigned parameter_writes, event_counts[8], read_calls, watchdog_feeds;\n'
    'static uint8_t result_types[128];')
PREFIX = PREFIX.replace('type==SCB_RESULT', '(type==SCB_RESULT||type==SCB_LOG_DATA)')
PREFIX = PREFIX.replace('cached[id][n]=0;', 'cached[id][n]=0;result_types[id]=type;')
PREFIX = PREFIX.replace('*t=SCB_RESULT;', '*t=result_types[id];')
PREFIX = PREFIX.replace('if(log_read_error)return 1;', '++read_calls;if(log_read_error)return 1;')
PREFIX = PREFIX.replace('void BSP_WDT_Feed(void){}', 'void BSP_WDT_Feed(void){++watchdog_feeds;}')
PREFIX += tuning.MAIN.split('static int16_t expected')[0]

MAIN = r'''

static datalog_result_t read_export_wire(uint32_t id,uint32_t offset,uint32_t length,
    uint8_t *payload,uint16_t *n)
{
    const uint8_t *raw;
    uint16_t raw_length;
    datalog_result_t result=Datalog_ReadExport(id,offset,length,&raw,&raw_length);
    *n=raw_length;
    if(result==DATALOG_RESULT_OK)*n=SCB_EncodeLogData(payload,offset,raw,raw_length);
    return result;
}

static uint8_t payload[968], copy[968];
static datalog_export_status_t info, after;
static datalog_meta_t meta;
static void stopped_log(unsigned count)
{
    healthy();assert(Lifecycle_TryStart());
    for(unsigned i=0;i<count;++i){App_SampleISR();if(i%8==7)Datalog_Service();}
    App_RequestStop(APP_STOP_OFF_TRACK);App_Loop();assert(App_IsStopped());
}
static void export_all(const char *path)
{
    FILE *out=fopen(path,"wb");assert(out);
    assert(Datalog_BeginExport(&info)==DATALOG_RESULT_OK);
    uint32_t offset=0;uint16_t n;unsigned blocks=0;
    while(offset<info.total_bytes)
    {
        uint32_t length=info.total_bytes-offset;
        if(length>960)length=960;
        if(!offset)length=1;
        assert(read_export_wire(info.transfer_id,offset,length,payload,&n)==DATALOG_RESULT_OK);
        assert(n==length+8&&SCB_ReadU32(payload)==offset&&SCB_ReadU16(payload+4)==length);
        assert(SCB_ReadU16(payload+6)==0&&!memcmp(payload+8,logs+offset,length));
        assert(fwrite(payload+8,1,length,out)==length);
        if(blocks++%17==0)
        {
            unsigned reads=read_calls;memcpy(copy,payload,n);Datalog_GetExportStatus(&after);
            uint32_t crc=after.crc32,next=after.next_offset;
            assert(read_export_wire(info.transfer_id,offset,length,payload,&n)==DATALOG_RESULT_OK);
            assert(reads==read_calls&&!memcmp(copy,payload,n));
            Datalog_GetExportStatus(&after);assert(after.crc32==crc&&after.next_offset==next);
        }
        offset+=length;
    }
    fclose(out);uint32_t total,crc;
    assert(Datalog_EndExport(info.transfer_id,&total,&crc)==DATALOG_RESULT_OK&&!Datalog_IsExporting());
    printf("CRC %s %u %u\n",path,total,crc);
}
int main(void)
{
    memset(params,255,sizeof params);App_Init();
    unsigned id=send("GET_LOG_META");status(id,"OK");printf("JSON %s\n",cached[id]);
    assert(Datalog_BeginExport(&info)==DATALOG_RESULT_NOT_FOUND);
    assert(Lifecycle_TryStart());unsigned reads=read_calls;
    id=send("GET_LOG_META");status(id,"OK");assert(read_calls==reads);printf("JSON %s\n",cached[id]);
    assert(Datalog_BeginExport(&info)==DATALOG_RESULT_BUSY);
    id=send("GET_STATUS");status(id,"OK");assert(read_calls==reads);
    printf("STATUS %s\n",cached[id]);
    App_ControlISR();App_SampleISR();App_SampleISR();App_ControlISR();App_SampleISR();
    App_RequestStop(APP_STOP_OFF_TRACK);assert(Datalog_BeginExport(&info)==DATALOG_RESULT_BUSY);
    App_Loop();Datalog_GetMeta(&meta);assert(meta.valid&&meta.written_records==3);
    reads=read_calls;id=send("GET_LOG_META");assert(read_calls==reads);printf("JSON %s\n",cached[id]);
    id=send("LOG_BEGIN");status(id,"OK");Datalog_GetExportStatus(&info);
    assert(info.total_bytes==544&&info.record_count==3);
    unsigned writes=log_writes,starts=timer_starts;
    assert(!Lifecycle_TryStart()&&timer_starts==starts&&log_writes==writes);
    assert(Datalog_Start()&&Datalog_Clear()&&log_writes==writes);
    Datalog_GetMeta(&meta);assert(meta.valid&&meta.written_records==3);
    command.transfer_id=info.transfer_id;id=send("LOG_END");status(id,"BUSY");
    command.offset=0;command.length=32;id=send("LOG_READ");assert(result_types[id]==SCB_LOG_DATA);
    assert(lengths[id]==40&&!memcmp(cached[id]+8,logs,32));
    command.target_request_id=id;id=send("GET_REQUEST_RESULT");status(id,"OK");
    printf("JSON %s\n",cached[id]);
    uint16_t n;
    assert(read_export_wire(info.transfer_id,0,32,payload,&n)==DATALOG_RESULT_OK);
    assert(read_export_wire(info.transfer_id,0,31,payload,&n)==DATALOG_RESULT_INVALID_ARG);
    assert(read_export_wire(info.transfer_id,33,1,payload,&n)==DATALOG_RESULT_INVALID_ARG);
    assert(read_export_wire(info.transfer_id,0xffffffffu,960,payload,&n)==DATALOG_RESULT_OUT_OF_RANGE);
    assert(read_export_wire(info.transfer_id,32,0,payload,&n)==DATALOG_RESULT_OUT_OF_RANGE);
    assert(read_export_wire(info.transfer_id,32,961,payload,&n)==DATALOG_RESULT_OUT_OF_RANGE);
    assert(read_export_wire(info.transfer_id+1,32,1,payload,&n)==DATALOG_RESULT_NOT_FOUND);
    assert(read_export_wire(info.transfer_id,32,512,payload,&n)==DATALOG_RESULT_OK);
    id=send("LOG_END");status(id,"OK");assert(!Datalog_IsExporting());
    /* 独立重新导出同一原字节流，Python核对CRC和每条时间轴。 */
    export_all(TEST_SMALL);
    assert(Datalog_BeginExport(&info)==DATALOG_RESULT_OK);log_read_error=1;
    unsigned feeds=watchdog_feeds;assert(read_export_wire(info.transfer_id,0,1,payload,&n)==DATALOG_RESULT_IO_ERROR);
    assert(!Datalog_IsExporting()&&watchdog_feeds>=feeds+2);log_read_error=0;
    /* 10秒边界、回绕、重复块不能续租。 */
    now=0xffffff00u;assert(Datalog_BeginExport(&info)==DATALOG_RESULT_OK);
    now=0xffffff00u+9999u;assert(Datalog_IsExporting());
    now=0xffffff00u+10000u;assert(!Datalog_IsExporting());
    now=100;assert(Datalog_BeginExport(&info)==DATALOG_RESULT_OK);
    assert(read_export_wire(info.transfer_id,0,1,payload,&n)==DATALOG_RESULT_OK);uint32_t progress=now-1;
    now=progress+9999;assert(read_export_wire(info.transfer_id,0,1,payload,&n)==DATALOG_RESULT_OK);
    now=progress+10000;assert(!Datalog_IsExporting());
    assert(Datalog_BeginExport(&info)==DATALOG_RESULT_OK);CommandDispatch_ResetSession();assert(!Datalog_IsExporting());
    assert(Datalog_Clear()==0);id=send("GET_LOG_META");printf("JSON %s\n",cached[id]);
    stopped_log(0);export_all(TEST_EMPTY);
    /* 最大87336条，保持原容量停车及实际已写时间轴。 */
    stopped_log(87336);Datalog_GetMeta(&meta);assert(meta.written_records==87336);
    export_all(TEST_MAX);
    /* 先写8条，后2条写失败：尾包保留pending，导出仅已确认前缀。 */
    healthy();assert(Lifecycle_TryStart());
    for(unsigned i=0;i<10;++i){App_SampleISR();if(i==7)Datalog_Service();}
    log_write_error=1;App_RequestStop(APP_STOP_OFF_TRACK);App_Loop();log_write_error=0;
    assert(Datalog_GetStatus()->written==8&&Datalog_GetStatus()->pending==2);
    assert(Datalog_GetStatus()->fault==DATALOG_FAULT_WRITE);
    export_all(TEST_PREFIX);assert(Datalog_GetStatus()->fault==DATALOG_FAULT_WRITE);
    puts("PASS: real log meta/export, full capacity, time axis, locks, CRC, repeat, failure, lease");
    return 0;
}
'''

if __name__ == '__main__':
    with tempfile.TemporaryDirectory(prefix='spi_export_') as directory:
        work = Path(directory)
        files = {name: work / (name.lower() + '.bin') for name in
                 ('TEST_SMALL', 'TEST_EMPTY', 'TEST_MAX', 'TEST_PREFIX')}
        defines = ''.join('#define ' + name + ' ' + json.dumps(str(path)) + '\n'
                          for name, path in files.items())
        output = dual.app_tests(work, defines + PREFIX + MAIN, real_tuning=True)
        objects = [json.loads(line[5:]) for line in output.splitlines() if line.startswith('JSON ')]
        status = json.loads(next(line[7:] for line in output.splitlines() if line.startswith('STATUS ')))
        assert status['running'] and status['log_valid'] and not status['export_active']
        assert status['run_id'] and status['written_records'] == 0
        assert objects[0]['log_valid'] is False and objects[0]['header_hex'] == ''
        assert objects[1]['written_records'] == 0 and len(objects[1]['header_hex']) == 512
        assert objects[2]['written_records'] == 3
        assert objects[3]['offset'] == 0 and objects[3]['length'] == 32
        assert objects[4]['log_valid'] is False
        expected = (3, 0, 87336, 8)
        crcs = [line.rsplit(' ', 2)[1:] for line in output.splitlines() if line.startswith('CRC ')]
        for path, count, result in zip(files.values(), expected, crcs):
            data = path.read_bytes()
            assert len(data) == 256 + 96 * count == int(result[0])
            assert zlib.crc32(data) == int(result[1])
            assert data[:4] == b'SCL2'
            ticks = [struct.unpack_from('<I', data, 256 + 96*i)[0] for i in range(count)]
            sequences = [struct.unpack_from('<I', data, 256 + 96*i + 68)[0] for i in range(count)]
            assert all(a < b for a, b in zip(ticks, ticks[1:]))
            assert all(a <= b for a, b in zip(sequences, sequences[1:]))
        small = files['TEST_SMALL'].read_bytes()
        assert [struct.unpack_from('<I', small, 256+96*i+68)[0] for i in range(3)] == [1, 1, 2]
        assert bytes.fromhex(objects[2]['header_hex']) == files['TEST_SMALL'].read_bytes()[:256]
        print(output.splitlines()[-1])
