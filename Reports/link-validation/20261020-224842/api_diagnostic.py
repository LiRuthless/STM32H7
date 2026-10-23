import sys,pathlib,time,json,socket,threading,datetime,struct,zlib,queue
out=pathlib.Path(sys.argv[1]);sys.path.insert(0,str(pathlib.Path('Tools/SmartCarLink').resolve()))
import scb_client
lock=threading.RLock()
trace=(out/'diagnostic-wire.jsonl').open('a',encoding='utf-8')
def note(event,**kw):
    with lock:
        obj=dict(time=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat(),mono_ns=time.monotonic_ns(),event=event,**kw)
        trace.write(json.dumps(obj,ensure_ascii=False)+'\n');trace.flush()
        if event not in ('tcp_rx_bytes','tcp_tx_bytes'): print(json.dumps(obj,ensure_ascii=True),flush=True)
real_create=socket.create_connection
class RecordedSocket:
    def __init__(self,s): self.s=s
    def __getattr__(self,k): return getattr(self.s,k)
    def recv(self,n,*a):
        b=self.s.recv(n,*a);note('tcp_rx_bytes',raw_hex=b.hex());return b
    def send(self,b,*a):
        n=self.s.send(b,*a);note('tcp_tx_bytes',raw_hex=bytes(b[:n]).hex());return n
def create(*a,**kw):
    s=real_create(*a,**kw);note('tcp_connected',local=s.getsockname(),peer=s.getpeername());return RecordedSocket(s)
scb_client.socket.create_connection=create
c=scb_client.DeviceClient(timeout=5,local_event_handler=lambda e:note('client_event',value=e))
results=[]
def request(op,args=None):
    start=time.monotonic();rid=c.session._next_request_id
    if not c.connected:
        note('query_reconnect_hello_ack',value=c.connect())
    try:
        r=c.request_final(op,args or {});obj=dict(op=op,request_id=rid,elapsed_ms=(time.monotonic()-start)*1000,result=r);results.append(obj);note('request_result',**obj);return r
    except Exception as e:
        obj=dict(op=op,request_id=rid,elapsed_ms=(time.monotonic()-start)*1000,error=repr(e));results.append(obj);note('request_error',**obj);return None
c.session._next_request_id=10
try:
    note('hello_ack',value=c.connect())
    request('GET_PARAMS',{'mode':1,'gear':0})
    request('GET_LOG_META')
    request('GET_STATUS')
    request('SUBSCRIBE',{'udp_port':5001})
    if c.connected:
        c.start_udp();start=time.monotonic();count=0
        while time.monotonic()-start<10:
            try:
                stamp,raw,packet=c.telemetry_queue.get(timeout=.25);count+=1
                note('udp_packet',recv_time_ns=stamp,raw_hex=raw.hex())
            except queue.Empty:pass
        note('udp_observation',seconds=time.monotonic()-start,packets=count,udp_stats=c.udp_stats,producer_state='unknown_no_STM32_status')
    c.close();note('client_active_close');time.sleep(.4)
    note('reconnect_hello_ack',value=c.connect())
    request('SUBSCRIBE',{'udp_port':5001});request('GET_STATUS')
except Exception as e:note('fatal_error',error=repr(e))
finally:
    c.close();note('closed');(out/'diagnostic-results.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8');trace.close()
