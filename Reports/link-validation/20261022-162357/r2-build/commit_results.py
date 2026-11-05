"""Commit only task validation documents, preserving shared checkout/index."""
from pathlib import Path
import os
import subprocess

root = Path(__file__).resolve().parents[4]
evidence = Path(__file__).resolve().parent
branch = "refs/heads/feature/16-spi-dma-recovery"
paths = ["SmartCar/specs/features/16-spi-wifi-link/requirements.md",
         "SmartCar/specs/features/16-spi-wifi-link/plan.md",
         "SmartCar/specs/features/16-spi-wifi-link/validation.md",
         "SmartCar/specs/roadmap.md", "SmartCar/CHANGELOG.md"]
index = evidence / "validation.index"
env = dict(os.environ, GIT_INDEX_FILE=str(index))


def git(*args, input=None, isolated=True):
    return subprocess.check_output(["git", *args], cwd=root,
                                   env=env if isolated else None, input=input)


assert git("diff", "--cached", "--name-only", isolated=False).strip() == b""
parent = git("rev-parse", branch).decode().strip()
assert parent == (evidence / "implementation-commit.txt").read_text().strip()
try:
    git("read-tree", parent)
    git("add", "--", *paths)
    changed = git("diff", "--cached", "--name-only", parent).decode().splitlines()
    assert sorted(changed) == sorted(paths), changed
    git("diff", "--cached", "--check", parent)
    (evidence / "validation-staged.diff").write_bytes(git("diff", "--cached", parent))
    tree = git("write-tree").decode().strip()
    commit = git("commit-tree", tree, "-p", parent,
                 input="docs(phase16): 记录DMA修复验证及实机待验边界\n".encode()).decode().strip()
    git("update-ref", branch, commit, parent)
    (evidence / "validation-commit.txt").write_text(commit + "\n", encoding="utf-8")
    print(commit)
    print(git("show", "--stat", "--oneline", commit).decode("utf-8"))
finally:
    if index.exists():
        index.unlink()
assert git("diff", "--cached", "--name-only", isolated=False).strip() == b""
