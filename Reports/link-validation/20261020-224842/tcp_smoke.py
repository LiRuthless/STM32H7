import sys, socket, time, json, struct, zlib, pathlib, datetime
out=pathlib.Path(sys.argv[1]); log=[]
def note(event, **kw):
    obj=dict(time=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat(), monotonic=time.monotonic(), event=event, **kw)
    log.append(obj); (out/'tcp-smoke.json').write_text(json.dumps(log,ensure_ascii=False,indent=2),encoding='utf-8'); print(json.dumps(obj,ensure_ascii=True),flush=True)
def recv_frame(s,label,timeout=3):
    s.settimeout(timeout); data=b''; target=32; start=time.monotonic()
    while len(data)<target:
        chunk=s.recv(target-len(data))
        if not chunk: note('eof',connection=label,partial_hex=data.hex()); return None
        data+=chunk
        if len(data)==32:
            h=struct.unpack('<4sBBHHHIIIII',data); target=32+h[4]
            if target>1024: raise ValueError('oversize')
    h=struct.unpack('<4sBBHHHIIIII',data[:32]); crc=zlib.crc32(data[32:],zlib.crc32(data[:28]))&0xffffffff
    body=json.loads(data[32:]) if h[1]==1 and h[2] in (1,2,0x11,0x12) else None
    note('rx_frame',connection=label,elapsed_ms=(time.monotonic()-start)*1000,raw_hex=data.hex(),header=list(h[1:]),crc_valid=crc==h[-1],body=body)
    assert h[0]==b'SCB1' and crc==h[-1]
    return body
payload=b'{"role":"pc","protocol":1}'
head=struct.pack('<4sBBHHHIIII',b'SCB1',1,1,0,len(payload),0,0,0,0,0)
hello=head+struct.pack('<I',zlib.crc32(payload,zlib.crc32(head))&0xffffffff)+payload
for attempt in range(2):
    s=None
    try:
        start=time.monotonic(); s=socket.create_connection(('192.168.4.1',5000),timeout=3)
        note('tcp_connected',attempt=attempt,local=s.getsockname(),peer=s.getpeername(),elapsed_ms=(time.monotonic()-start)*1000)
        s.sendall(hello); note('tx_hello',attempt=attempt,raw_hex=hello.hex())
        body=recv_frame(s,'primary')
        if body is None: continue
        if body.get('status')=='BUSY': note('existing_control_busy'); break
        s.settimeout(2.2)
        try:
            data=s.recv(1024); note('hold_eof' if not data else 'hold_data',raw_hex=data.hex())
        except socket.timeout: note('hold_timeout_alive',seconds=2.2)
        try:
            with socket.create_connection(('192.168.4.1',5000),timeout=3) as second:
                recv_frame(second,'secondary'); second.settimeout(1)
                try: note('secondary_after_busy',raw_hex=second.recv(1024).hex())
                except socket.timeout: note('secondary_eof_not_observed')
        except Exception as exc: note('secondary_error',error=repr(exc))
        s.settimeout(1)
        try: note('primary_after_busy',raw_hex=s.recv(1024).hex())
        except socket.timeout: note('primary_still_open')
    except Exception as exc: note('error',attempt=attempt,error=repr(exc))
    finally:
        if s: s.close(); note('client_active_close',attempt=attempt)
    time.sleep(.3)
