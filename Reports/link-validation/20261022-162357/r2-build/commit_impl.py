"""Commit exact task paths on isolated branch without changing shared HEAD/index."""
from pathlib import Path
import os
import subprocess

root = Path(__file__).resolve().parents[4]
evidence = Path(__file__).resolve().parent
branch = "refs/heads/feature/16-spi-dma-recovery"
paths = ["SmartCar/BSP/bsp_wifi.c", "SmartCar/tests/spi_wifi_state.py"]
index = evidence / "implementation.index"
env = dict(os.environ, GIT_INDEX_FILE=str(index))


def git(*args, input=None, isolated=True):
    return subprocess.check_output(["git", *args], cwd=root,
                                   env=env if isolated else None, input=input)


assert git("diff", "--cached", "--name-only", isolated=False).strip() == b""
parent = git("rev-parse", branch).decode().strip()
assert parent == "12fe38b80719e2a8531da9cd72cf39e430571d95"
try:
    git("read-tree", parent)
    git("add", "--", *paths)
    changed = git("diff", "--cached", "--name-only", parent).decode().splitlines()
    assert sorted(changed) == sorted(paths), changed
    git("diff", "--cached", "--check", parent)
    diff = git("diff", "--cached", parent, "--", *paths)
    (evidence / "implementation-staged.diff").write_bytes(diff)
    tree = git("write-tree").decode().strip()
    commit = git("commit-tree", tree, "-p", parent,
                 input="fix(phase16): 完整回收SPI异常后的双向DMA\n".encode()).decode().strip()
    git("update-ref", branch, commit, parent)
    (evidence / "implementation-commit.txt").write_text(commit + "\n", encoding="utf-8")
    print(commit)
    print(git("show", "--stat", "--oneline", commit).decode("utf-8"))
finally:
    if index.exists():
        index.unlink()
assert git("diff", "--cached", "--name-only", isolated=False).strip() == b""
