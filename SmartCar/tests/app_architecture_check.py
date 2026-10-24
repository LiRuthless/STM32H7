"""Check the final five-category App boundaries against real sources.

Quoted BSP interfaces are terminal edges: HAL implementation behind BSP remains
outside App. This structural check supplements, rather than replaces, C replay.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET

from app_inventory import inventory, mask_comments

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / 'App'
CATEGORIES = {'system', 'sensing', 'control', 'services', 'hmi'}
ROOT_FILES = {'app.c', 'app.h', 'app_config.h', 'app_types.h'}
PROTOCOL = {'scb_frame.h', 'scb_json.h', 'scb_command_codec.h', 'scb_link.h'}
ALGORITHMS = {'direction_controller', 'angle_controller', 'speed_controller',
              'pid', 'legacy_pid', 'tuning_values'}
PARAMETERS = {'parameter_bank', 'parameter_store', 'parameter_image'}
LOGGING = {'datalog', 'run_record', 'logger', 'log_store', 'log_export'}
STANDARD_HEADERS = {name+'.h' for name in (
    'assert', 'complex', 'ctype', 'errno', 'fenv', 'float', 'inttypes', 'iso646',
    'limits', 'locale', 'math', 'setjmp', 'signal', 'stdarg', 'stdbool', 'stddef',
    'stdint', 'stdio', 'stdlib', 'string', 'tgmath', 'time', 'wchar', 'wctype')}


def check():
    assert {p.name for p in APP.iterdir() if p.is_dir()} == CATEGORIES
    assert {p.name for p in APP.iterdir() if p.is_file()} == ROOT_FILES
    assert not [p for group in CATEGORIES for p in (APP/group).rglob('*') if p.is_dir()]
    paths = sorted(p for p in APP.rglob('*') if p.suffix in ('.c', '.h'))
    assert len({p.name for p in paths}) == len(paths), 'ambiguous module basename'
    sources = {p.relative_to(APP).as_posix(): mask_comments(p.read_text(encoding='utf-8'))
               for p in paths}
    graph = {}
    bsp_edges = []
    objects = []
    functions = []
    for name, source in sources.items():
        for include in re.findall(r'^\s*#\s*include\s*<([^>]+)>', source, re.M):
            assert include in STANDARD_HEADERS, (name, 'non-standard angle include', include)
        assert not re.search(r'\b(?:HAL_\w+|LL_\w+|\w+TypeDef|h(?:tim|spi|adc|uart)\d)\b', source), name
        assert not re.search(r'\b(?:RCC|GPIO[A-K]|TIM\d+|SPI\d+|ADC\d+|USART\d+|DMA\d+_Stream\d+|DWT|CoreDebug|NVIC|SCB)\s*->', source), name
        assert not re.search(r'\b__(?:DMB|DSB|ISB|WFI|WFE|disable_irq|enable_irq|get_PRIMASK|set_PRIMASK)\s*\(', source), name
        assert not re.search(r'\b(?:malloc|calloc|realloc|free)\s*\(', source), name
        graph[name] = []
        for include in re.findall(r'^\s*#\s*include\s*"([^"]+)"', source, re.M):
            if include in sources:
                assert '/' in include or include in ROOT_FILES, (name, include)
                graph[name].append(include)
            else:
                assert (ROOT/'BSP'/include).is_file(), (name, 'non-App/non-BSP include', include)
                assert include.startswith('bsp'), (name, include)
                bsp_edges.append((name, include))
        for row in inventory('App/'+name, (APP/name).read_text(encoding='utf-8')):
            if row['kind'] == 'object':
                assert row.get('const') or 'static' in row['storage'], row
                objects.append(row)
            elif row['kind'] == 'function':
                functions.append(row)

    def closure(name):
        seen = set()
        todo = [name]
        while todo:
            item = todo.pop()
            if item not in seen:
                seen.add(item)
                todo.extend(graph[item])
        return seen

    for name in sources:
        dependencies = closure(name)
        if name.startswith('sensing/'):
            assert not [p for p in dependencies if p.startswith(('system/', 'hmi/', 'services/'))], name
        if Path(name).stem in ALGORITHMS:
            assert not [p for p in dependencies if p.startswith(('sensing/', 'system/', 'services/', 'hmi/'))
                        or Path(p).stem == 'motor'], name
        if Path(name).stem in PARAMETERS:
            assert not [p for p in dependencies if p.startswith(('control/', 'system/', 'hmi/'))
                        or Path(p).name in PROTOCOL], name
        if name.startswith('system/') or Path(name).stem in LOGGING or Path(name).stem == 'tuning':
            assert not [p for p in dependencies if Path(p).name in PROTOCOL], name
        if Path(name).stem == 'tuning':
            assert not [p for p in dependencies if p.startswith(('system/', 'services/', 'hmi/'))], name
        if Path(name).stem in LOGGING:
            assert not re.search(r'\b(?:SCB_\w+|scb_\w+_t)\b', sources[name]), name
        if re.search(r'\bBSP_Flash_Log(?:Read|Write|Erase)\s*\(', sources[name]):
            assert name == 'services/log_store.c', name

    projects = []
    project_ids = {}
    ordinal = 0
    project = ET.parse(ROOT/'MDK-ARM/SmartCar.uvprojx')
    for group_number, group in enumerate(project.findall('.//Groups/Group'), 1):
        for file in group.findall('./Files/File'):
            ordinal += 1
            path = (ROOT/'MDK-ARM'/file.findtext('FilePath')).resolve()
            project_ids[path] = (group_number, ordinal)
    for filename, group_path, path_tag in [('SmartCar.uvprojx', './/Groups/Group', 'FilePath'),
                                         ('SmartCar.uvoptx', './Group', 'PathWithFileName')]:
        project = ET.parse(ROOT/'MDK-ARM'/filename)
        app_sources = []
        for group in project.findall(group_path):
            for file in group.findall('./Files/File' if filename.endswith('uvprojx') else './File'):
                path = (ROOT/'MDK-ARM'/file.findtext(path_tag)).resolve()
                if APP in path.parents:
                    assert path.is_file(), (filename, path)
                    if filename.endswith('uvoptx'):
                        assert (int(file.findtext('GroupNumber')),
                                int(file.findtext('FileNumber'))) == project_ids[path], (filename, path)
                    if path.suffix == '.c':
                        expected = 'App' if path.parent == APP else 'App/'+path.parent.name
                        assert group.findtext('GroupName') == expected, (filename, path)
                        app_sources.append(path)
        assert set(app_sources) == {p for p in paths if p.suffix == '.c'}, filename
        assert all(n == 1 for n in Counter(app_sources).values()), filename
        projects.append(filename)
    return dict(source_sha256={name: hashlib.sha256((APP/name).read_bytes()).hexdigest() for name in sources},
                include_graph=graph, bsp_boundary_edges=bsp_edges, file_objects=objects,
                functions=functions, projects=projects,
                limitations='Structural evidence; ownership/ISR consistency requires semantic review and real C replay.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    report = check()
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(f"PASS final App boundaries: {len(report['source_sha256'])} C/H; "
          f"{len(report['functions'])} function definitions; five categories, private objects, both Keil manifests")
