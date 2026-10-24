"""Check G4 ownership and exact historical run-event statements across module moves."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "App"
BASE = "6125495"
TOKEN = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\w+|[^\s]', re.S)


def tokens(text):
    return [m[0] for m in TOKEN.finditer(text) if not m[0].startswith(('/*', '//'))]


def body(text, name):
    items = tokens(text)
    start = next(i for i, value in enumerate(items) if value == name and items[i+1] == '(')
    opening = items.index('{', start)
    level = 1
    for end in range(opening+1, len(items)):
        if items[end] == '{':
            level += 1
        elif items[end] == '}':
            level -= 1
            if not level:
                return items[opening+1:end]
    raise AssertionError(name)


def ordered(items, names):
    positions = [items.index(name) for name in names]
    assert positions == sorted(positions), names


def main():
    dispatch = (APP / 'services/command_dispatch.c').read_text(encoding='utf-8')
    for kind, filename in [('Started', 'lifecycle'), ('Stopped', 'safety')]:
        old = subprocess.run(['git', 'show', f'{BASE}:SmartCar/App/system/{filename}.c'],
                             cwd=ROOT, check=True, capture_output=True, text=True,
                             encoding='utf-8').stdout
        # Compare every historical declaration, zero initialization, read and assignment.
        match = re.search(r'    \{\n        telemetry_status_t upload;.*?\n    \}', old, re.S)
        assert match, kind
        expected = tokens(match[0])[1:-1]
        if kind == 'Stopped':
            expected = tokens(match[0].replace('(uint8_t)s_stop_reason', 'reason'))[1:-1]
        assert body(dispatch, 'CommandDispatch_PublishRun'+kind) == expected, kind
    assert body(dispatch, 'CommandDispatch_InitLink') == tokens('SCB_Init();')
    assert body(dispatch, 'CommandDispatch_ServiceLink') == tokens('SCB_Service();')
    lifecycle = (APP / 'system/lifecycle.c').read_text(encoding='utf-8')
    ordered(body(lifecycle, 'Lifecycle_Init'),
            ['CommandDispatch_InitLink', 'Tuning_Init', 'CommandDispatch_Init'])
    ordered(body(lifecycle, 'Lifecycle_TryStart'),
            ['BSP_Sampler_Start', 'CommandDispatch_PublishRunStarted', 'BSP_WDT_Init'])
    mode = body(lifecycle, 'Lifecycle_TrySetMode')
    ordered(mode, ['App_IsStopped', 'Param_TryFlushPending', 'Tracking_SetMode',
                   'Control_ResetRunState', 'Motor_ResetRunState', 'PID_ResetAll',
                   'IMU_ResetRunState', 'Param_ApplyCurrentDirection'])
    assert mode[:11] == tokens('if (mode > 1) { return TRACKING_ARG; }')
    tracking = (APP / 'control/tracking.c').read_text(encoding='utf-8')
    assert body(tracking, 'Tracking_SetMode') == tokens('s_tracking.mode = mode;')
    assert 'Param_TryFlushPending' not in tokens(tracking)
    assert 'Param_ApplyCurrentDirection' not in tokens(tracking)
    for path in (APP / 'system').glob('*'):
        if path.suffix in ('.c', '.h'):
            assert not re.search(r'\b(?:SCB_\w+|scb_\w+)\b', ' '.join(tokens(path.read_text(encoding='utf-8')))), path
    for name in ['parameter_bank', 'parameter_store']:
        text = (APP / f'services/{name}.c').read_text(encoding='utf-8')
        assert not re.search(r'#include "(?:control|system|hmi)/', text), name
    core = (APP / 'control/tuning.c').read_text(encoding='utf-8')
    assert not re.search(r'\b(?:SCB_|scb_|Datalog_|Telemetry_|Menu_|Param_)[A-Za-z0-9_]*',
                         ' '.join(tokens(core)))
    print('PASS G4 boundaries: historical run-event statements, link wrappers, runtime mode order and owner dependencies')


if __name__ == '__main__':
    main()
