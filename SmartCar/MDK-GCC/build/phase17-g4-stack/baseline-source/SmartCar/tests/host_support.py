"""Build real firmware C with a native compiler; keep products outside the repository."""
from app_fixture import APP
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def build(work, sources, name="test", flags=()):
    binary = work / (name + ".exe")
    subprocess.run([
        "gcc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
        *flags, "-I", str(work), "-I", str(APP),
        "-I", str(ROOT / "BSP"), *map(str, sources), "-lm", "-o", str(binary)
    ], check=True)
    return binary
