import pathlib,json,struct,zlib,sys,hashlib
out=pathlib.Path(sys.argv[1]);frames=[]
for filename in ['api-wire.jsonl','diagnostic-wire.jsonl']:
    buffers={'tcp_tx_bytes':b'','tcp_rx_bytes':b''}
    for line in (out/filename).read_text(encoding='utf-8').splitlines():
        e=json.loads(line);kind=e['event']
        if kind=='tcp_connected':buffers={k:b'' for k in buffers}
        if kind not in buffers:continue
        buffers[kind]+=bytes.fromhex(e['raw_hex'])
        while len(buffers[kind])>=32:
            h=struct.unpack('<4sBBHHHIIIII',buffers[kind][:32]);n=32+h[4]
            if len(buffers[kind])<n:break
            raw,buffers[kind]=buffers[kind][:n],buffers[kind][n:]
            frames.append(dict(file=filename,direction=kind,time=e['time'],magic_ok=h[0]==b'SCB1',version=h[1],type=h[2],flags=h[3],payload_len=h[4],session_id=h[6],seq=h[7],ack_seq=h[8],request_id=h[9],crc_ok=(zlib.crc32(raw[32:],zlib.crc32(raw[:28]))&0xffffffff)==h[-1],body=json.loads(raw[32:]) if h[4] and h[2] in [1,2,16,17,18] else None))
(out/'independent-frame-audit.json').write_text(json.dumps(frames,indent=2,ensure_ascii=False),encoding='utf-8')
print(json.dumps({'frames':len(frames),'bad_crc':sum(not x['crc_ok'] for x in frames),'tx_commands':[(x['body']['op'],x['request_id']) for x in frames if x['direction']=='tcp_tx_bytes' and x['type']==16],'rx_result_count':sum(x['type']==17 for x in frames)},ensure_ascii=True))
