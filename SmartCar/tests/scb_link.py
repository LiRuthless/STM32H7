"""Exercise the real C stop-and-wait/session/request cache with a simulated peer."""
from pathlib import Path
import json
import shutil
import subprocess
import tempfile
from host_support import ROOT, build

BSP = r'''
#include <stdint.h>
#include "bsp_wifi.h"
uint32_t HAL_GetTick(void);
uint32_t BSP_Time_EnterCritical(void);
void BSP_Time_ExitCritical(uint32_t);
void BSP_Time_MemoryBarrier(void);
'''
HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "scb_link.c"
#include "telemetry.c"
static unsigned calls, resets;
static uint32_t clock_ms;
static unsigned permit, start_return;
static uint8_t input[1024], output[1024], saved[1024], result[992];
uint32_t HAL_GetTick(void){return clock_ms;}
uint32_t BSP_Time_EnterCritical(void){return 0;}
void BSP_Time_ExitCritical(uint32_t m){(void)m;}
void BSP_Time_MemoryBarrier(void){}
void BSP_WiFi_Init(void){}
void BSP_WiFi_Service(void){}
const uint8_t *BSP_WiFi_GetRx(void){return NULL;}
uint8_t *BSP_WiFi_GetTx(void){return output;}
void BSP_WiFi_ReleaseRx(void){}
uint8_t BSP_WiFi_TryExchange(void){if(start_return)permit=0;return (uint8_t)start_return;}
void BSP_WiFi_GetStatus(bsp_wifi_status_t *s){memset(s,0,sizeof(*s));s->permits=permit;}
static void handler(uint32_t id,const scb_command_t *c)
{
    const char *text="{\"op\":\"SET_PARAMS\",\"status\":\"RECEIVED\"}";
    (void)c;++calls;
    assert(SCB_QueueResult(id,SCB_RESULT,(const uint8_t*)text,(uint16_t)strlen(text),0));
}
static void reset(void){++resets;}
static void peer(uint8_t type,uint32_t session,uint32_t seq,uint32_t ack,uint32_t id,const char *text)
{
    scb_frame_t f={type,type==SCB_COMMAND?1:0,(uint16_t)strlen(text),0,session,seq,ack,id,(const uint8_t*)text};
    assert(SCB_EncodeFrame(input,&f));
}
static void handshake(uint32_t now)
{
    scb_frame_t f;
    SCB_ResetSession(now);
    assert(SCB_PrepareFrame(output,now));
    assert(SCB_DecodeFrame(output,1024,&f)&&f.type==SCB_HELLO&&!f.seq&&!f.session_id);
    peer(SCB_HELLO_ACK,123,0,0,0,"{\"protocol\":1,\"session_id\":123,\"link_ready\":false}");
    assert(SCB_ReceiveFrame(input,now));assert(SCB_ReceiveFrame(input,now));
    assert(SCB_PrepareFrame(output,now));
    assert(SCB_DecodeFrame(output,1024,&f)&&f.type==SCB_IDLE&&f.session_id==123&&f.seq==1);
    assert(s_link.ready);
}
int main(int argc,char **argv)
{
    scb_frame_t f;uint16_t length;uint8_t type;
    SCB_SetHandlers(handler,reset);SCB_Init();handshake(0);
    peer(SCB_COMMAND,123,7,0,9,"{\"op\":\"TUNE_ENABLE\",\"args\":{}}");
    assert(SCB_ReceiveFrame(input,1)&&calls==1&&s_link.ack_seq==7);
    assert(SCB_ReceiveFrame(input,2)&&calls==1);
    peer(SCB_IDLE,123,1000,0,0,"");assert(SCB_ReceiveFrame(input,3));
    peer(SCB_COMMAND,123,7,0,9,"{\"op\":\"TUNE_ENABLE\",\"args\":{}}");
    assert(SCB_ReceiveFrame(input,4)&&calls==1);
    peer(SCB_COMMAND,123,8,0,9,"{\"op\":\"TUNE_ENABLE\",\"args\":{}}");
    assert(SCB_ReceiveFrame(input,5)&&calls==1);
    peer(SCB_COMMAND,123,9,0,9,"{\"op\":\"TUNE_DISABLE\",\"args\":{}}");
    assert(SCB_ReceiveFrame(input,6)&&calls==1&&s_reject_ready);
    assert(SCB_PrepareFrame(output,6));assert(SCB_DecodeFrame(output,1024,&f));
    assert(f.type==SCB_RESULT&&strstr((const char*)f.payload,"INVALID_ARG"));
    memcpy(saved,output,1024);unsigned reliable_seq=f.seq;
    peer(SCB_IDLE,123,1001,reliable_seq+1,0,"");assert(SCB_ReceiveFrame(input,7)&&s_link.reliable);
    for(unsigned i=1;i<=3;++i){
        peer(SCB_IDLE,123,1001+i,0,0,"");assert(SCB_ReceiveFrame(input,6+200*i));
        assert(SCB_PrepareFrame(output,6+200*i));assert(!memcmp(saved,output,1024));
        assert(s_link.retries==i);
    }
    peer(SCB_IDLE,123,1008,0,0,"");assert(SCB_ReceiveFrame(input,806));
    assert(SCB_PrepareFrame(output,806));assert(SCB_DecodeFrame(output,1024,&f)&&f.type==SCB_HELLO);
    handshake(1000);calls=0;
    for(unsigned i=1;i<=32;++i){
        peer(SCB_COMMAND,123,i,0,i,"{\"op\":\"TUNE_ENABLE\",\"args\":{}}");
        assert(SCB_ReceiveFrame(input,1000+i));
    }
    assert(calls==32);
    peer(SCB_COMMAND,123,33,0,33,"{\"op\":\"TUNE_ENABLE\",\"args\":{}}");
    assert(SCB_ReceiveFrame(input,1033)&&calls==32);
    assert(SCB_PrepareFrame(output,1033)&&SCB_DecodeFrame(output,1024,&f));
    assert(strstr((const char*)f.payload,"BUSY"));
    peer(SCB_IDLE,123,34,f.seq,0,"");assert(SCB_ReceiveFrame(input,1034)&&!s_link.reliable);
    assert(SCB_GetRequestResult(1,&type,result,&length)&&type==SCB_RESULT);
    const char *final="{\"op\":\"SET_PARAMS\",\"status\":\"APPLIED\"}";
    assert(SCB_QueueResult(1,SCB_RESULT,(const uint8_t*)final,(uint16_t)strlen(final),1));
    assert(!SCB_GetRequestResult(999,&type,result,&length));
    peer(SCB_COMMAND,123,35,0,99,"{\"op\":\"TUNE_ENABLE\",\"args\":{},\"op\":\"TUNE_DISABLE\"}");
    assert(!SCB_ReceiveFrame(input,1100)&&s_link.ack_seq==33);
    peer(SCB_COMMAND,124,36,0,99,"{\"op\":\"TUNE_ENABLE\",\"args\":{}}");
    assert(!SCB_ReceiveFrame(input,1101));
    handshake(2000);
    assert(!SCB_GetRequestResult(1,&type,result,&length));
    assert(SCB_PrepareFrame(output,2799)&&s_link.ready);
    assert(SCB_PrepareFrame(output,2800)&&!s_link.ready);
    assert(SCB_DecodeFrame(output,1024,&f)&&f.type==SCB_HELLO);
    assert(!SCB_PrepareFrame(output,2999));assert(SCB_PrepareFrame(output,3000));
    handshake(0xfffffff0u);assert(SCB_PrepareFrame(output,0x30)&&s_link.ready);
    s_link.next_seq=0xffffff00u;assert(SCB_PrepareFrame(output,0x31)&&!s_link.ready);
    handshake(4000);
    peer(SCB_COMMAND,123,1,0,1,"{\"op\":\"TUNE_ENABLE\",\"args\":{}}");
    assert(SCB_ReceiveFrame(input,4000));permit=1;start_return=0;clock_ms=4001;
    SCB_Service();unsigned prepared_seq=s_reliable_seq;
    assert(s_prepared&&s_link.reliable&&!s_link.retries);
    clock_ms=4199;SCB_Service();assert(s_reliable_seq==prepared_seq&&!s_link.retries);
    start_return=1;clock_ms=4200;SCB_Service();assert(!s_prepared&&s_sent_at==4200);
    assert(SCB_PrepareFrame(output,4399)&&!s_link.retries);
    assert(SCB_PrepareFrame(output,4400)&&s_link.retries==1);
    handshake(4500);assert(!SCB_PrepareFrame(output,4504));
    assert(SCB_PrepareFrame(output,4505)&&SCB_DecodeFrame(output,1024,&f)&&f.type==SCB_IDLE);
    unsigned next_seq=s_link.next_seq;permit=0;clock_ms=4509;SCB_Service();
    assert(s_link.next_seq==next_seq);permit=1;clock_ms=4700;SCB_Service();
    assert(s_link.next_seq==next_seq+1&&!s_prepared); /* 阻塞后首个服务即尝试。 */
    handshake(5000);Telemetry_BeginRun();
    datalog_record_t record={0};scb_event_t event={.kind=SCB_EVENT_RUN_STARTED,.run_id=1};
    for(unsigned i=0;i<10;++i){record.tick=1000+i;Telemetry_PushRecord(&record);}
    for(unsigned i=0;i<32;++i){event.tick=i;assert(SCB_QueueEvent(&event));}
    assert(!SCB_QueueEvent(&event)&&!SCB_QueueEvent(&event)&&s_gap_count==2);
    peer(SCB_COMMAND,123,1,0,1,"{\"op\":\"TUNE_ENABLE\",\"args\":{}}");
    assert(SCB_ReceiveFrame(input,5000));
    assert(SCB_PrepareFrame(output,5010)&&SCB_DecodeFrame(output,1024,&f)&&f.type==SCB_RESULT);
    peer(SCB_IDLE,123,2,f.seq,0,"");assert(SCB_ReceiveFrame(input,5011));
    for(unsigned i=0;i<33;++i){
        assert(SCB_PrepareFrame(output,5012+i)&&SCB_DecodeFrame(output,1024,&f)&&f.type==SCB_EVENT);
        if(i==32)assert(strstr((const char*)f.payload,"EVENT_GAP")&&strstr((const char*)f.payload,"\"dropped_events\":2"));
        peer(SCB_IDLE,123,3+i,f.seq,0,"");assert(SCB_ReceiveFrame(input,5012+i));
    }
    assert(SCB_PrepareFrame(output,5050)&&SCB_DecodeFrame(output,1024,&f)&&f.type==SCB_TELEMETRY);
    assert(f.record_count==10&&f.payload_len==984&&SCB_ReadU32(f.payload+20)==1);
    Telemetry_Init();clock_ms=0;Telemetry_BeginRun();Telemetry_SetRevision(3);
    for(unsigned i=0;i<256;++i){record.tick=i;Telemetry_PushRecord(&record);}
    Telemetry_PushRecord(&record);telemetry_status_t status;Telemetry_GetStatus(&status);
    assert(status.queued==256&&status.dropped==1&&status.next_index==257);
    uint16_t records;
    assert(!Telemetry_Service(result,&length,&records,9));
    assert(Telemetry_Service(result,&length,&records,10)&&records==10&&length==984);
    assert(SCB_ReadU32(result)==0&&SCB_ReadU32(result+4)==3&&SCB_ReadU32(result+8)==1);
    Telemetry_ResetSession();assert(s_tail==s_head&&s_revision==3&&s_next_index==257);
    s_head=s_tail=0xfffffff8u;s_next_index=0;
    for(unsigned i=0;i<12;++i)Telemetry_PushRecord(&record);
    assert(s_head==4&&s_head-s_tail==12);
    assert(Telemetry_Service(result,&length,&records,20)&&records==10);
    Telemetry_ResetSession();s_last_packet=20;
    Telemetry_SetRevision(4);Telemetry_PushRecord(&record);Telemetry_SetRevision(5);Telemetry_PushRecord(&record);
    assert(Telemetry_Service(result,&length,&records,21)&&records==1&&SCB_ReadU32(result+4)==4);
    assert(!Telemetry_Service(result,&length,&records,22));
    assert(Telemetry_Service(result,&length,&records,31)&&records==1&&SCB_ReadU32(result+4)==5);
    unsigned previous_run=s_run_id;Telemetry_BeginRun();assert(s_run_id==previous_run+1&&!s_next_index);
    Telemetry_PushRecord(&record);s_slots[s_tail&255].run_id=previous_run;
    unsigned drops=s_consumer_dropped;assert(!Telemetry_Service(result,&length,&records,100)&&s_consumer_dropped==drops+1);
    /* 固定10条记录作为输入，独立Python随后核验完整984B输出。 */
    assert(argc==2);Telemetry_Init();s_run_id=7;s_revision=3;s_next_index=10;s_producer_dropped=2;
    for(unsigned i=0;i<10;++i){memcpy(&record,vector_records+96*i,96);Telemetry_PushRecord(&record);}
    assert(Telemetry_Service(result,&length,&records,10000)&&records==10);
    FILE *packet=fopen(argv[1],"wb");assert(packet);fwrite(result,1,length,packet);fclose(packet);
    assert(resets>=8);
    puts("PASS V-06/07/08/09/10 real C: sessions, ACK, 32 reservations/events+gap, SPSC256, wrap, versions");
    return 0;
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix="scb_link_") as temp:
        work = Path(temp)
        shutil.copyfile(ROOT / "App/scb_link.c", work / "scb_link.c")
        shutil.copyfile(ROOT / "App/telemetry.c", work / "telemetry.c")
        (work / "bsp.h").write_text(BSP)
        vectors = json.loads((Path(__file__).parent / "scb_vectors.json").read_text())
        vector = bytes.fromhex(vectors["TELEMETRY_10_HEX"])
        records = vector[32 + 24:]
        prefix = "static const unsigned char vector_records[]={" + ",".join(map(str, records)) + "};\n"
        (work / "test.c").write_text(prefix + HARNESS)
        binary = build(work, [work / "test.c", ROOT / "App/scb_protocol.c"])
        packet = work / "packet.bin"
        subprocess.run([str(binary), str(packet)], check=True)
        expected = bytearray(vector[32:])
        expected[12:16] = b"\0" * 4  # ESP count is filled by the bridge, STM32 starts at zero.
        assert packet.read_bytes() == expected


if __name__ == "__main__":
    main()
