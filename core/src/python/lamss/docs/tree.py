# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Writes the staged site so that `mkdocs serve` sees only real changes.

Rewriting an unchanged file would retrigger serve's watcher on every rebuild, forever.
"""

import os
from pathlib import Path


class StagedTree:
    def __init__(self, root):
        self.root = Path(root)
        self.written = set()

    def write_bytes(self, rel, data):
        path = self.root / rel
        self.written.add(Path(rel).as_posix())
        if path.is_file() and path.read_bytes() == data:
            return
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)

    def write_text(self, rel, text):
        self.write_bytes(rel, text.encode())

    def copy(self, source, rel):
        self.write_bytes(rel, Path(source).read_bytes())

    def remove_stale(self):
        """Deletes files staged by an earlier run that this run did not write."""
        if not self.root.is_dir():
            return
        for dirpath, dirnames, filenames in os.walk(self.root, topdown=False):
            for name in filenames:
                path = Path(dirpath) / name
                if path.relative_to(self.root).as_posix() not in self.written:
                    path.unlink()
            if not os.listdir(dirpath) and Path(dirpath) != self.root:
                os.rmdir(dirpath)
