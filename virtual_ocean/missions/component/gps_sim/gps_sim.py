# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""uSimGPS: GPS fixes while the simulated vehicle is near the surface."""

from lamss.mission import Component, Process

from missions.component.interprocess import Interprocess


class GpsSim(Component):
    requires = [Interprocess]
    sim_only = True
    params = dict(max_depth=5, interval=2, min_time=15)
    processes = [Process("uSimGPS", "uSimGPS last.moos")]
    fragments = {"last.moos": ["uSimGPS.moos.j2"]}
