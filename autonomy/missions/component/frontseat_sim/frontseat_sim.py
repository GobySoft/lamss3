# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Simulates the vehicle under a frontseat component: its dynamics, autopilot and computer."""

from lamss.mission import Component

from missions.component.interprocess import Interprocess


class FrontseatSim(Component):
    requires = [Interprocess]
    sim_only = True
