# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""check_loiter_waypoints' geometry and run logic, on synthetic tracks rather than mission logs."""

import math
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent))
from check_loiter_waypoints import longest_run, reached, waypoints  # noqa: E402


def track_through(points, depth=15.0, steps=20):
    """A track along straight legs through points, at a constant depth."""
    track = []
    for (x0, y0), (x1, y1) in zip(points, points[1:]):
        for i in range(steps):
            f = i / steps
            track.append({"x": x0 + f * (x1 - x0), "y": y0 + f * (y1 - y0), "depth": depth})
    return track


def test_radial_waypoints_start_half_a_segment_clockwise_of_north():
    # MOOS-IvP's format=radial: with 6 points the first is at a compass bearing of 30 degrees
    wpts = waypoints(0, 0, 200, 6)
    bearings = [math.degrees(math.atan2(x, y)) % 360 for x, y in wpts]
    assert bearings == pytest.approx([30, 90, 150, 210, 270, 330])
    assert all(math.hypot(x, y) == pytest.approx(200) for x, y in wpts)


@pytest.mark.parametrize("step", [1, -1], ids=["clockwise", "counterclockwise"])
def test_only_the_commanded_direction_counts(step):
    wpts = waypoints(0, 0, 200, 6)
    order = [wpts[(k * step) % 6] for k in range(5)]  # four legs in one direction
    visits = reached(track_through([(0, 0)] + order), wpts, 25)
    assert len(longest_run(visits, track_through([(0, 0)] + order), 6, step, 15, 2)) == 5
    assert len(longest_run(visits, track_through([(0, 0)] + order), 6, -step, 15, 2)) == 1


def test_a_depth_excursion_between_waypoints_breaks_the_run():
    wpts = waypoints(0, 0, 200, 6)
    track = track_through([(0, 0)] + wpts[:5])
    # leave the commanded depth on the leg from waypoint 1 to waypoint 2
    for r in track[2 * 20 + 5:2 * 20 + 10]:
        r["depth"] = 25.0
    visits = reached(track, wpts, 25)
    assert [k for k, _ in visits] == [0, 1, 2, 3, 4]
    assert [k for k, _ in longest_run(visits, track, 6, 1, 15, 2)] == [2, 3, 4]
