# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Plan 10's M1 gate for the sonar chain: the last track resolution received over acomms lies
within TOLERANCE degrees of the true bearing from macrura to the simulated target at that time.

Truth is uSimTargets' NODE_REPORT (tgt_1_sim) in sensor_sim's goby_logger log; macrura's position
and the track are the NODE_REPORTs resolution logged. Reads the newest run of an instance:

    check_track_bearing.py <missions>/log/<instance> --tolerance 10 [-l <proto lib>]...
"""

import argparse
import datetime
import math
import re
import subprocess
import sys
import tempfile
from pathlib import Path

REPORT = re.compile(r"\| lamss::moos::NODE_REPORT \| lamss.core.LegacyNodeReport \| (.+?) \| (.*)$")


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
                                "time": logged.timestamp()})
        return reports


def nearest(reports: list[dict], name: str, t: float) -> dict:
    candidates = [r for r in reports if r["name"] == name]
    if not candidates:
        sys.exit(f"no NODE_REPORT named {name}")
    return min(candidates, key=lambda r: abs(r["time"] - t))


def bearing(frm: dict, to: dict) -> float:
    return math.degrees(math.atan2(to["x"] - frm["x"], to["y"] - frm["y"])) % 360


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("instance_log", type=Path)
    parser.add_argument("--tolerance", type=float, default=10.0)
    parser.add_argument("--vehicle", default="macrura")
    parser.add_argument("--topside", default="resolution")
    parser.add_argument("--target", default="tgt_1_sim")
    parser.add_argument("-l", "--load_shared_library", action="append", default=[])
    args = parser.parse_args()

    runs = sorted((d for d in args.instance_log.iterdir() if d.is_dir()), key=lambda d: d.stat().st_mtime)
    if not runs:
        sys.exit(f"no runs in {args.instance_log}")
    run = runs[-1]

    def log(platform, subsystem):
        files = sorted((run / platform / subsystem).glob(f"{platform}_*.goby"))
        files = [f for f in files if not f.name.endswith("_latest.goby")]
        if not files:
            sys.exit(f"no goby_logger log in {run / platform / subsystem}")
        return files[0]

    topside = node_reports(log(args.topside, "main"), args.load_shared_library)
    truth = node_reports(log(args.vehicle, "sensor_sim"), args.load_shared_library)
    tracks = [r for r in topside if r["type"] == "track" and r["name"].startswith(f"{args.vehicle}_track")]
    if not tracks:
        sys.exit(f"{args.topside} received no track from {args.vehicle} in {run}")
    track = tracks[-1]
    own = nearest(topside, args.vehicle, track["time"])
    target = nearest(truth, args.target, track["time"])
    estimated, true = bearing(own, track), bearing(own, target)
    error = (estimated - true + 180) % 360 - 180
    print(f"{run.name}: {track['name']} at ({track['x']:.0f}, {track['y']:.0f}); {args.vehicle} at "
          f"({own['x']:.0f}, {own['y']:.0f}); target at ({target['x']:.0f}, {target['y']:.0f}); "
          f"bearing {estimated:.1f} deg, true {true:.1f} deg, error {error:+.1f} deg "
          f"(tolerance {args.tolerance:g})")
    return 0 if abs(error) <= args.tolerance else 1


if __name__ == "__main__":
    sys.exit(main())
