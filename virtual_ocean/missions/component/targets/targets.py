# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""uSimTargets: acoustic targets on straight lines, here the cruise's `target` (a
`lamss.virtual_ocean.LAMSS_SIMULATE_TARGET` as a dict) from the start; more arrive as TGT_STATE_IN."""

from lamss.mission import Component, Process

from missions.component.interprocess import Interprocess


class Targets(Component):
    requires = [Interprocess]
    sim_only = True
    processes = [Process("uSimTargets", "uSimTargets last.moos")]
    fragments = {"last.moos": ["uSimTargets.moos.j2"]}
