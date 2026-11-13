# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Reading a mission test's goby_logger logs: the newest run of an instance, and its NODE_REPORTs."""

import datetime
import re
import subprocess
import sys
import tempfile
from pathlib import Path

REPORT = re.compile(r"\| lamss::moos::NODE_REPORT \| lamss.core.LegacyNodeReport \| (.+?) \| (.*)$")


def newest_run(instance_log: Path) -> Path:
    runs = sorted((d for d in instance_log.iterdir() if d.is_dir()), key=lambda d: d.stat().st_mtime)
    if not runs:
        sys.exit(f"no runs in {instance_log}")
    return runs[-1]


def goby_log(run: Path, platform: str, subsystem: str) -> Path:
    files = sorted((run / platform / subsystem).glob(f"{platform}_*.goby"))
    files = [f for f in files if not f.name.endswith("_latest.goby")]
    if not files:
        sys.exit(f"no goby_logger log in {run / platform / subsystem}")
    return files[0]


def node_reports(goby_file: Path, libs: list[str]) -> list[dict]:
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "log.txt"
        command = ["goby_log_tool", "-i", str(goby_file), "-o", str(out), "--format", "DEBUG_TEXT"]
        for lib in libs:
            command += ["--load_shared_library", lib]
        subprocess.run(command, check=True, capture_output=True)
        reports = []
        for line in out.read_text(errors="replace").splitlines():
            m = REPORT.search(line)
            if not m:
                continue
            fields = dict(re.findall(r'(\w+): ("[^"]*"|[-0-9.e]+)', m.group(2)))
            fields = {k: v.strip('"') for k, v in fields.items()}
            if "x" in fields and "y" in fields:
                # the logger's timestamp: report times come from different clocks when warped
                logged = datetime.datetime.strptime(m.group(1), "%Y-%b-%d %H:%M:%S.%f")
                reports.append({"name": fields.get("name"), "type": fields.get("type"),
                                "x": float(fields["x"]), "y": float(fields["y"]),
                                "depth": float(fields.get("depth", "nan")),
                                "time": logged.timestamp()})
        return reports
