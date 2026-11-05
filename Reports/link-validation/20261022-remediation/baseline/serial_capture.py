import sys, time, pathlib, json

out = pathlib.Path(sys.argv[1])
seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 60.0
meta = {'port': 'COM3', 'baud': 115200, 'dtr': False, 'rts': False,
        'write_bytes': 0, 'reset_requested': False, 'seconds_requested': seconds}
try:
    import serial
    s = serial.Serial(port=None, baudrate=115200, timeout=.2)
    s.dtr = False
    s.rts = False
    s.port = 'COM3'
    s.open()
    start = time.monotonic()
    data = b''
    while time.monotonic() - start < seconds:
        data += s.read(4096)
    s.close()
    meta.update(seconds=time.monotonic() - start, bytes=len(data))
    (out / 'esp32-passive-serial.bin').write_bytes(data)
    (out / 'esp32-passive-serial.txt').write_text(data.decode('utf-8', errors='replace'), encoding='utf-8')
except Exception as e:
    meta['error'] = repr(e)
(out / 'esp32-passive-serial-meta.json').write_text(json.dumps(meta, indent=2), encoding='utf-8')
print(json.dumps(meta))
