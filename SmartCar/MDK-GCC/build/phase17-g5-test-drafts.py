"""Prepare current-side host-fixture adaptations without editing real tests."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'MDK-GCC/build/phase17-g5-draft/tests'
OUT.mkdir(parents=True, exist_ok=True)
CODEC = ['checksum', 'scb_frame', 'scb_json', 'scb_command_codec']
LOGGING = ['run_record', 'logger', 'log_store', 'log_export']


def save(name, content):
    (OUT / name).write_text(content, encoding='utf-8', newline='\n')


for name in ('scb_protocol.py', 'scb_link.py', 'datalog_v2.py', 'dual_mode_tracking.py',
             'spi_log_export.py', 'phase16_check.py'):
    value = (ROOT / 'tests' / name).read_text(encoding='utf-8')
    value = value.replace('#include "scb_protocol.h"', '#include "scb_command_codec.h"')
    # Each standalone compiler links the actual split implementations. Existing
    # tuning_values.c stays real and is not replaced with a test stub.
    value = value.replace('APP / "scb_protocol.c"',
                          ', '.join('APP / "' + module + '.c"' for module in CODEC))
    value = value.replace("str(APP / 'scb_protocol.c')",
                          ','.join("str(APP / '" + module + ".c')" for module in CODEC))
    if name == 'datalog_v2.py':
        value = value.replace('#include "datalog.c"',
                              '\n'.join('#include "' + module + '.c"' for module in LOGGING))
        value = value.replace("'datalog.c',", ','.join(repr(module + '.c') for module in LOGGING) + ',')
    if name == 'dual_mode_tracking.py':
        value = value.replace("'datalog',", ', '.join(repr(module) for module in LOGGING) + ',')
        value = value.replace("'scb_protocol')", ', '.join(repr(module) for module in CODEC) + ')')
    if name == 'spi_log_export.py':
        # The original wire assertions stay intact. The fixture immediately
        # consumes borrowed raw bytes through the same real production codec.
        value = re.sub(r'(Datalog_(?:Begin|Read|End)Export\([^\n]*?==)SCB_(\w+)',
                       lambda m: m[1] + 'DATALOG_RESULT_' + m[2], value)
        value = value.replace('Datalog_ReadExport(', 'read_export_wire(')
        helper = r'''
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
'''
        value = value.replace('static uint8_t payload[968], copy[968];',
                              helper + '\nstatic uint8_t payload[968], copy[968];')
    if name == 'phase16_check.py':
        value = value.replace("for name in ('scb_protocol',", 'for name in ('
                              + ', '.join(repr(module) for module in CODEC) + ',')
        value = value.replace("'App/services/scb_protocol.c',",
                              ', '.join(repr('App/services/' + module + '.c') for module in CODEC) + ',')
        value = value.replace("'App/services/datalog.c',",
                              ', '.join(repr('App/services/' + module + '.c') for module in LOGGING) + ',')
    compile(value, name, 'exec')
    save(name, value)

print('Prepared six current-side fixture drafts; historical source and expectation strings retained.')
