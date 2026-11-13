# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Plan 10's M1 gate for the vehicle: macrura reaches COUNT consecutive waypoints of its loiter in
the commanded direction, holding DEPTH within TOLERANCE metres from the first to the last.

The waypoints are those of the loiter behaviour's `format=radial` polygon: SEGMENTS points at
RADIUS around CENTER, the first at half a segment clockwise of north. A waypoint is reached when
one of macrura's own NODE_REPORTs (pNodeReporter, in main's goby_logger log) comes within CAPTURE
metres of it. Reads the newest run of an instance:

    check_loiter_waypoints.py <missions>/log/<instance> --clockwise [-l <proto lib>]...
"""

import argparse
import math
import sys
from pathlib import Path

from mission_logs import goby_log, newest_run, node_reports


def waypoints(x: float, y: float, radius: float, segments: int) -> list[tuple[float, float]]:
    step = 360.0 / segments
    return [(x + radius * math.sin(math.radians(step * (k + 0.5))),
             y + radius * math.cos(math.radians(step * (k + 0.5)))) for k in range(segments)]


def reached(track: list[dict], wpts: list[tuple[float, float]], capture: float) -> list[tuple[int, int]]:
    """(waypoint index, track index) each time the vehicle comes within capture of a new waypoint."""
    visits = []
    for i, r in enumerate(track):
        for k, (wx, wy) in enumerate(wpts):
            if math.hypot(r["x"] - wx, r["y"] - wy) <= capture and (not visits or visits[-1][0] != k):
                visits.append((k, i))
    return visits


def longest_run(visits: list[tuple[int, int]], track: list[dict], segments: int, step: int,
                depth: float, tolerance: float) -> list[tuple[int, int]]:
    def held(i, j):
        return all(abs(r["depth"] - depth) <= tolerance for r in track[i:j + 1])

    best, run = [], []
    for v in visits:
        follows = run and v[0] == (run[-1][0] + step) % segments and held(run[-1][1], v[1])
        run = run + [v] if follows else ([v] if held(v[1], v[1]) else [])
        if len(run) > len(best):
            best = run
    return best


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("instance_log", type=Path)
    parser.add_argument("--vehicle", default="macrura")
    parser.add_argument("--center", type=float, nargs=2, default=(0.0, 0.0), metavar=("X", "Y"))
    parser.add_argument("--radius", type=float, default=200.0)
    parser.add_argument("--segments", type=int, default=6)
    direction = parser.add_mutually_exclusive_group(required=True)
    direction.add_argument("--clockwise", dest="clockwise", action="store_true")
    direction.add_argument("--counterclockwise", dest="clockwise", action="store_false")
    parser.add_argument("--capture", type=float, default=25.0)
    parser.add_argument("--count", type=int, default=3)
    parser.add_argument("--depth", type=float, default=15.0)
    parser.add_argument("--tolerance", type=float, default=2.0)
    parser.add_argument("-l", "--load_shared_library", action="append", default=[])
    args = parser.parse_args()

    run = newest_run(args.instance_log)
    track = [r for r in node_reports(goby_log(run, args.vehicle, "main"), args.load_shared_library)
             if r["name"] == args.vehicle]
    if not track:
        sys.exit(f"no NODE_REPORT from {args.vehicle} in {run}")

    wpts = waypoints(*args.center, args.radius, args.segments)
    visits = reached(track, wpts, args.capture)
    # compass bearings increase clockwise, and so do the waypoint indices
    best = longest_run(visits, track, args.segments, 1 if args.clockwise else -1, args.depth,
                       args.tolerance)
    order = " -> ".join(str(k) for k, _ in visits)
    print(f"{run.name}: waypoints reached {order or '(none)'}; longest "
          f"{'clockwise' if args.clockwise else 'counterclockwise'} run at {args.depth:g} +/- "
          f"{args.tolerance:g} m: {' -> '.join(str(k) for k, _ in best) or '(none)'} "
          f"({len(best)}, need {args.count})")
    return 0 if len(best) >= args.count else 1


if __name__ == "__main__":
    sys.exit(main())
