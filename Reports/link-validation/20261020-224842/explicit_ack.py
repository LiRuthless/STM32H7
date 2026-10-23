import sys,pathlib,socket,struct,zlib,json,time,datetime
out=pathlib.Path(sys.argv[1]);f=(out/'explicit-ack.jsonl').open('w',encoding='utf-8');seq=0;ack=0;session=0
def note(e,**kw):
    x=dict(event=e,time=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat(),mono_ns=time.monotonic_ns(),**kw);f.write(json.dumps(x)+'\n');f.flush();print(json.dumps(x),flush=True)
def frame(t,body=None,rid=0):
    global seq
    if t==1:sid=0;n=0
    else:seq+=1;sid=session;n=seq
    p=json.dumps(body,separators=(',',':')).encode() if body is not None else b''
    h=struct.pack('<4sBBHHHIIII',b'SCB1',1,t,1 if t==16 else 0,len(p),0,sid,n,ack,rid)
    return h+struct.pack('<I',zlib.crc32(h+p)&0xffffffff)+p
buf=b''
def rx(s,timeout):
    global buf
    deadline=time.monotonic()+timeout
    while True:
        if len(buf)>=32:
            h=struct.unpack('<4sBBHHHIIIII',buf[:32]);n=32+h[4]
            if n>1024:raise ValueError('oversize')
            if len(buf)>=n:
                raw,buf=buf[:n],buf[n:];valid=zlib.crc32(raw[:28]+raw[32:])&0xffffffff==h[-1]
                note('rx',raw_hex=raw.hex(),crc_valid=valid,type=h[2],seq=h[7],ack=h[8],request_id=h[9],body=json.loads(raw[32:]) if h[4] else None)
                assert valid and h[0]==b'SCB1'
                return h,json.loads(raw[32:]) if h[4] else None
        left=deadline-time.monotonic()
        if left<=0:raise socket.timeout()
        s.settimeout(left);chunk=s.recv(4096)
        if not chunk:raise EOFError('peer EOF')
        buf+=chunk
def tx(s,b):s.sendall(b);note('tx',raw_hex=b.hex())
try:
    with socket.create_connection(('192.168.4.1',5000),timeout=3) as s, socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as udp:
        udp.bind(('0.0.0.0',5001));udp.settimeout(.01)
        tx(s,frame(1,{'role':'pc','protocol':1}));h,b=rx(s,3);session=b['session_id'];assert b['link_ready']
        tx(s,frame(16,{'op':'SUBSCRIBE','args':{'udp_port':5001}},100));h,b=rx(s,3)
        assert b=={'op':'SUBSCRIBE','status':'OK'} and h[9]==100 and h[3]==1
        ack=h[7];tx(s,frame(3));start=time.monotonic();packets=0;duplicates=0
        while time.monotonic()-start<10:
            try:
                h,b=rx(s,.2)
                if h[3]&1:ack=h[7];tx(s,frame(3));duplicates+=1
            except socket.timeout:pass
            try:
                raw,src=udp.recvfrom(2048);packets+=1;note('udp',raw_hex=raw.hex(),source=src)
            except socket.timeout:pass
        note('explicit_ack_hold_pass',seconds=time.monotonic()-start,udp_packets=packets,repeated_reliable_frames=duplicates,session_id=session,link_ready_initial=True)
        # Further HELLO/BUSY on a second socket; first must remain usable for IDLE.
        saved=buf
        with socket.create_connection(('192.168.4.1',5000),timeout=3) as second:
            buf=b'';h,b=rx(second,3);assert b.get('status')=='BUSY';second.settimeout(1);assert second.recv(1)==b''
        buf=saved;tx(s,frame(3));
        try:rx(s,.5)
        except socket.timeout:note('primary_open_after_busy')
        tx(s,frame(16,{'op':'GET_STATUS','args':{}},101))
        try:rx(s,1)
        except socket.timeout:note('GET_STATUS_no_ack_or_result',request_id=101,seconds=1)
except Exception as e:note('error',error=repr(e))
finally:f.close()
