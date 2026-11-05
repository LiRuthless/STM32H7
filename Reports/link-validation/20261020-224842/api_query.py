import sys,pathlib,time,json,socket,threading,datetime,struct,zlib,queue
out=pathlib.Path(sys.argv[1]);sys.path.insert(0,str(pathlib.Path('Tools/SmartCarLink').resolve()))
import scb_client
lock=threading.RLock()
trace=(out/'api-wire.jsonl').open('a',encoding='utf-8')
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
    try:
        r=c.request_final(op,args or {});obj=dict(op=op,request_id=rid,elapsed_ms=(time.monotonic()-start)*1000,result=r);results.append(obj);note('request_result',**obj);return r
    except Exception as e:
        obj=dict(op=op,request_id=rid,elapsed_ms=(time.monotonic()-start)*1000,error=repr(e));results.append(obj);note('request_error',**obj);return None
try:
    note('hello_ack',value=c.connect())
    status=request('GET_STATUS')
    if status and status.get('status')=='OK':
        request('GET_PARAMS',{'mode':status['mode'],'gear':status['gear']})
        request('GET_LOG_META')
        with real_create(('192.168.4.1',5000),timeout=3) as s:
            s.settimeout(3);raw=b''
            while len(raw)<32:
                chunk=s.recv(32-len(raw))
                if not chunk:break
                raw+=chunk
            if len(raw)==32:
                h=struct.unpack('<4sBBHHHIIIII',raw);left=h[4]
                while left:
                    chunk=s.recv(left)
                    if not chunk:break
                    raw+=chunk;left-=len(chunk)
                note('second_connection',raw_hex=raw.hex(),body=json.loads(raw[32:]),crc_valid=(zlib.crc32(raw[32:],zlib.crc32(raw[:28]))&0xffffffff)==h[-1],eof=s.recv(1)==b'')
        request('GET_STATUS')
        c.start_udp();request('SUBSCRIBE',{'udp_port':5001})
        start=time.monotonic();received=[]
        while time.monotonic()-start<10:
            try:
                item=c.telemetry_queue.get(timeout=.25);received.append(repr(item))
            except queue.Empty:pass
        note('parked_udp_observation',seconds=time.monotonic()-start,queue_items=len(received),udp_stats=c.udp_stats)
        before=request('GET_STATUS');request('GET_PARAMS',{'mode':status['mode'],'gear':status['gear']})
        c.close();note('client_active_close');time.sleep(.5)
        note('reconnect_hello_ack',value=c.connect())
        request('SUBSCRIBE',{'udp_port':5001});after=request('GET_STATUS');request('GET_PARAMS',{'mode':status['mode'],'gear':status['gear']});request('GET_LOG_META')
        note('recovery_comparison',before=before,after=after,connected=c.connected,udp_stats=c.udp_stats)
except Exception as e:note('fatal_error',error=repr(e))
finally:
    c.close();note('closed');(out/'query-results.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8');trace.close()
