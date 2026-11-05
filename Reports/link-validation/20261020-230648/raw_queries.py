import json,pathlib,socket,sys,time,struct,zlib
out=pathlib.Path(sys.argv[1]);f=(out/'raw-queries.jsonl').open('w',encoding='utf-8')
def note(e,**kw):
 x=dict(event=e,time_ns=time.time_ns(),mono_ns=time.monotonic_ns(),**kw);f.write(json.dumps(x)+'\n');f.flush();print(json.dumps(x),flush=True)
def build(t,p=b'',sid=0,seq=0,ack=0,rid=0):
 h=struct.pack('<4sBBHHHIIII',b'SCB1',1,t,1 if t==16 else 0,len(p),0,sid,seq,ack,rid);return h+struct.pack('<I',zlib.crc32(h+p)&0xffffffff)+p
for rid,op,args in [(101,'GET_STATUS',{}),(102,'GET_PARAMS',{'mode':1,'gear':0}),(103,'GET_LOG_META',{})]:
 try:
  with socket.create_connection(('192.168.4.1',5000),timeout=3) as s:
   buffer=b''
   def read(timeout=2):
    nonlocal_dummy=0
    global buffer
    end=time.monotonic()+timeout
    while True:
     if len(buffer)>=32:
      h=struct.unpack('<4sBBHHHIIIII',buffer[:32]);n=32+h[4]
      if n>1024:raise ValueError('oversize')
      if len(buffer)>=n:
       raw,buffer=buffer[:n],buffer[n:];assert zlib.crc32(raw[:28]+raw[32:])&0xffffffff==h[-1]
       body=json.loads(raw[32:]) if h[4] else None;note('rx',raw_hex=raw.hex(),body=body,crc_valid=True);return h,body
     s.settimeout(max(.001,end-time.monotonic()));chunk=s.recv(4096)
     if not chunk:raise EOFError('peer closed')
     buffer+=chunk
   hello=build(1,b'{"role":"pc","protocol":1}');s.sendall(hello);note('tx',raw_hex=hello.hex());h,b=read();sid=b['session_id']
   if not b['link_ready'] or not sid:note('skipped_not_ready',op=op);continue
   p=json.dumps({'op':op,'args':args},separators=(',',':')).encode();seq=1;ack=0;raw=build(16,p,sid,seq,ack,rid);start=time.monotonic();s.sendall(raw);note('tx',op=op,raw_hex=raw.hex())
   for attempt in range(12):
    h,b=read(2)
    if h[3]&1:
     ack=h[7];seq+=1;idle=build(3,sid=sid,seq=seq,ack=ack);s.sendall(idle);note('tx_ack_idle',raw_hex=idle.hex())
    if h[2]==17 and h[9]==rid:note('business_result',op=op,request_id=rid,elapsed_ms=(time.monotonic()-start)*1000,result=b);break
 except Exception as e:note('error',op=op,request_id=rid,error=repr(e))
 time.sleep(.25)
f.close()
