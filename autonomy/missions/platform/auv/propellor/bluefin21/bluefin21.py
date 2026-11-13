# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""A Bluefin 21 with a LAMSS backseat. In simulation, frontseat_sim stands in for the vehicle."""

from lamss.mission import Subsystem, optional

from missions.component.frontseat.bluefin import BluefinFrontseat
from missions.component.frontseat_sim.bluefin_sim import BluefinFrontseatSim
from missions.component.health import Health
from missions.component.interprocess import Interprocess
from missions.component.logging import Logging
from missions.platform.auv.propellor import Propellor


class Bluefin21(Propellor):
    components = Propellor.components + [BluefinFrontseat]
    subsystems = Propellor.subsystems + [
        Subsystem("frontseat_sim", [Interprocess, Health, Logging, BluefinFrontseatSim, optional("gps_sim")],
                  mode="sim")]
