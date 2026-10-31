"""Resolve current App modules and preserve flat white-box harness overrides.

Only category prefixes on include directives are removed in the temporary copy.
The production tree is untouched; missing or ambiguous module names are errors.
"""
from pathlib import Path
import re
import tempfile

ROOT = Path(__file__).resolve().parents[1]
APP_ROOT = ROOT / "App"
CATEGORIES = ("system", "sensing", "control", "services", "hmi")


def app_source(name):
    """Locate one current C/H by basename, never falling back to historical files."""
    matches = list(APP_ROOT.rglob(name))
    if len(matches) != 1:
        raise ValueError(f"Expected one current App module {name!r}, found {matches}")
    return matches[0]


def flatten_includes(data):
    """Normalize only the approved category prefixes in C include directives."""
    return re.sub(rb'(?m)^(\s*#\s*include\s+")(' +
                  b'|'.join(item.encode() for item in CATEGORIES) + rb')/', rb'\1', data)


_fixture = tempfile.TemporaryDirectory(prefix="smartcar_app_")
APP = Path(_fixture.name)
for source in sorted(APP_ROOT.rglob("*")):
    if source.suffix in (".c", ".h"):
        resolved = app_source(source.name)
        (APP / source.name).write_bytes(flatten_includes(resolved.read_bytes()))
