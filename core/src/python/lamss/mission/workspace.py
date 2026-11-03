# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Where the checkout, its active toolboxes and the install prefix are."""

from __future__ import annotations

import os
from pathlib import Path
from typing import Optional

from .errors import MissionError
from .registry import Toolbox


def find_root(start: Optional[Path] = None) -> Path:
    """LAMSS_ROOT, else the nearest directory above `start` (default: cwd) holding `.lamss/`."""
    env = os.environ.get("LAMSS_ROOT")
    if env:
        root = Path(env)
        if not (root / ".lamss").is_dir():
            raise MissionError(f"LAMSS_ROOT={env} has no .lamss/ directory")
        return root.resolve()
    here = (start or Path.cwd()).resolve()
    for d in (here, *here.parents):
        if (d / ".lamss").is_dir():
            return d
    raise MissionError(f"not inside a lamss3 checkout (no .lamss/ above {here}); set LAMSS_ROOT")


def active_toolboxes(root: Path) -> list[str]:
    names = ["core"]
    listing = root / ".lamss" / "toolboxes"
    if listing.is_file():
        for line in listing.read_text().splitlines():
            line = line.strip()
            if line and not line.startswith("#") and line not in names:
                names.append(line)
    return names


def toolboxes(root: Path) -> list[Toolbox]:
    return [Toolbox(name, root / name / "missions") for name in active_toolboxes(root)]


def prefix(root: Path) -> Path:
    env = os.environ.get("LAMSS_PREFIX")
    return Path(env) if env else root / "build" / "usr"
