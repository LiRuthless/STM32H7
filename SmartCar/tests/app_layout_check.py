"""Check Phase 17 G1 path completeness, source preservation and project coverage."""
import argparse
from collections import Counter
import json
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

from app_fixture import APP, APP_ROOT, CATEGORIES, ROOT, app_source, flatten_includes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", default="4745aa2339209d5df9a739449685fc7f1cb66429", help="Committed firmware before G1 migration")
    parser.add_argument("--g1-equivalence", action="store_true")
    args = parser.parse_args()
    mapping = json.loads((ROOT / "specs/features/17-app-architecture/path-map.json").read_text(encoding="utf-8"))
    paths = sorted(path for path in APP_ROOT.rglob("*") if path.suffix in (".c", ".h"))
    assert len(paths) == len(list(APP.glob("*"))), "fixture must cover every production C/H"
    historical = subprocess.run(["git", "ls-tree", "-r", "--name-only", args.base, "SmartCar/App"],
                                cwd=ROOT.parent, capture_output=True, check=True).stdout.decode("utf-8")
    historical_names = {Path(name).name for name in historical.splitlines()
                        if Path(name).suffix in (".c", ".h")}
    removed={"control.c","control.h","element.c","element.h","roundabout.c","roundabout.h"}
    assert historical_names - removed <= {path.name for path in paths}, "all non-split modules retained"
    assert {v for k,v in mapping.items() if k not in removed} <= {path.relative_to(APP_ROOT).as_posix() for path in paths}
    for source in paths:
        relative = source.relative_to(APP_ROOT).as_posix()
        assert len(source.relative_to(APP_ROOT).parts) <= 2, source
        if source.parent != APP_ROOT:
            assert source.parent.name in CATEGORIES, source
        assert app_source(source.name) == source
        if source.name in mapping:
            assert relative == mapping[source.name], (source, mapping)
        current = flatten_includes(source.read_bytes())
        if args.g1_equivalence:
            original = subprocess.run(["git", "show", f"{args.base}:SmartCar/App/{source.name}"],
                                      cwd=ROOT.parent, capture_output=True, check=True).stdout
            assert current.replace(b"\r\n", b"\n") == original.replace(b"\r\n", b"\n"), source
        assert (APP / source.name).read_bytes() == current, source
    project = ET.parse(ROOT / "MDK-ARM/SmartCar.uvprojx")
    groups = project.findall(".//Groups/Group")
    names = [group.findtext("GroupName") for group in groups]
    for category in CATEGORIES:
        assert names.count(f"App/{category}") == 1, names
    project_sources = []
    for group in groups:
        for file in group.findall("Files/File"):
            path = (ROOT / "MDK-ARM" / file.findtext("FilePath")).resolve()
            if APP_ROOT in path.parents and path.suffix == ".c":
                assert path.is_file(), path
                expected = "App" if path.parent == APP_ROOT else f"App/{path.parent.name}"
                assert group.findtext("GroupName") == expected, path
                project_sources.append(path)
    actual = {path for path in paths if path.suffix == ".c"}
    assert set(project_sources) == actual, (set(project_sources) ^ actual)
    assert all(count == 1 for count in Counter(project_sources).values()), project_sources
    script = (ROOT / "MDK-GCC/build.sh").read_text(encoding="utf-8")
    assert "shopt -s nullglob" in script
    for category in CATEGORIES:
        assert f'"$PROJ"/App/{category}/*.c' in script, category
    print(f"PASS App structure: {len(paths)} C/H located; fixture exact; {len(actual)} Keil sources once; five GCC groups")


if __name__ == "__main__":
    main()
