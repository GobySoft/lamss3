# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""A flat bottom in isovelocity water: an environment that needs no data files."""

from lamss.mission import Environment


class Synthetic(Environment):
    params = dict(bathymetry=dict(kind="flat", depth=100.0), sound_speed=1500.0, currents=None, ice=None)
