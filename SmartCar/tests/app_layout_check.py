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
    parser.add_argument("--base", required=True, help="Committed firmware before G1 migration")
    args = parser.parse_args()
    mapping = json.loads((ROOT / "specs/features/17-app-architecture/path-map.json").read_text(encoding="utf-8"))
    paths = sorted(path for path in APP_ROOT.rglob("*") if path.suffix in (".c", ".h"))
    assert len(paths) == len(list(APP.glob("*"))), "fixture must cover every production C/H"
    historical = subprocess.run(["git", "ls-tree", "-r", "--name-only", args.base, "SmartCar/App"],
                                cwd=ROOT.parent, capture_output=True, check=True).stdout.decode("utf-8")
    historical_names = {Path(name).name for name in historical.splitlines()
                        if Path(name).suffix in (".c", ".h")}
    assert {path.name for path in paths} == historical_names, "G1 must preserve all baseline modules"
    assert set(mapping.values()) <= {path.relative_to(APP_ROOT).as_posix() for path in paths}
    for source in paths:
        relative = source.relative_to(APP_ROOT).as_posix()
        assert len(source.relative_to(APP_ROOT).parts) <= 2, source
        if source.parent != APP_ROOT:
            assert source.parent.name in CATEGORIES, source
        assert app_source(source.name) == source
        assert relative == mapping.get(source.name, source.name), (source, mapping)
        original = subprocess.run(["git", "show", f"{args.base}:SmartCar/App/{source.name}"],
                                  cwd=ROOT.parent, capture_output=True, check=True).stdout
        current = flatten_includes(source.read_bytes())
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
    print(f"PASS G1: {len(paths)} C/H preserved; fixture exact; {len(actual)} Keil sources once; five GCC groups")


if __name__ == "__main__":
    main()
