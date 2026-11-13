# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""A propeller-driven AUV, steered by pHelmIvP under pLamssMissionManager."""

from missions.component.ivp import Ivp
from missions.component.mission_manager import MissionManager
from missions.platform.auv import Auv


class Propellor(Auv):
    components = Auv.components + [MissionManager, Ivp]
