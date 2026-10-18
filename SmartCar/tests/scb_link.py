"""Exercise the real C stop-and-wait/session/request cache with a simulated peer."""
from pathlib import Path
import shutil
import subprocess
import tempfile
from host_support import ROOT, build

BSP = r'''
#include <stdint.h>
#include "bsp_wifi.h"
uint32_t HAL_GetTick(void);
'''
HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "scb_link.c"
static unsigned calls, resets;
static uint32_t clock_ms;
static unsigned permit, start_return;
static uint8_t input[1024], output[1024], saved[1024], result[992];
uint32_t HAL_GetTick(void){return clock_ms;}
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
int main(void)
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
    assert(resets>=8);
    puts("PASS V-06/07/08 real C sessions, 799/800ms, retries, exact ACK, 32 reservations, dedup");
    return 0;
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix="scb_link_") as temp:
        work = Path(temp)
        shutil.copyfile(ROOT / "App/scb_link.c", work / "scb_link.c")
        (work / "bsp.h").write_text(BSP)
        (work / "test.c").write_text(HARNESS)
        binary = build(work, [work / "test.c", ROOT / "App/scb_protocol.c"])
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
