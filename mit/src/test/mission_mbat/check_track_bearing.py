# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Plan 10's M1 gate for the sonar chain: the last track resolution received over acomms lies
within TOLERANCE degrees of the true bearing from macrura to the simulated target at that time.

Truth is uSimTargets' NODE_REPORT (tgt_1_sim) in sensor_sim's goby_logger log; macrura's position
and the track are the NODE_REPORTs resolution logged. Reads the newest run of an instance:

    check_track_bearing.py <missions>/log/<instance> --tolerance 20 [-l <proto lib>]...
"""

import argparse
import math
import sys
from pathlib import Path

from mission_logs import goby_log, newest_run, node_reports


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
    parser.add_argument("--tolerance", type=float, default=20.0)
    parser.add_argument("--vehicle", default="macrura")
    parser.add_argument("--topside", default="resolution")
    parser.add_argument("--target", default="tgt_1_sim")
    parser.add_argument("-l", "--load_shared_library", action="append", default=[])
    args = parser.parse_args()

    run = newest_run(args.instance_log)

    topside = node_reports(goby_log(run, args.topside, "main"), args.load_shared_library)
    truth = node_reports(goby_log(run, args.vehicle, "sensor_sim"), args.load_shared_library)
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
