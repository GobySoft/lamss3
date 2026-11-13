# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""MIT's Bluefin 21s, which tow the DURIP array and talk to the topside over acomms. In simulation
their sensor_sim subsystem simulates the array and the targets it hears, linked to main by
pMOOSLink."""

from lamss.mission import Subsystem

from missions.component.acomms.udp import UdpAcomms
from missions.component.health import Health
from missions.component.interprocess import Interprocess
from missions.component.logging import Logging
from missions.component.moos_link import MoosLink
from missions.component.passive_sonar_sim import PassiveSonarSim
from missions.component.sonar.passive import PassiveSonar
from missions.component.targets import Targets
from missions.platform.auv.propellor.bluefin21 import Bluefin21

SENSOR_SIM_LINK = MoosLink(
    inputs=["NAV_X", "NAV_Y", "NAV_Z", "NAV_DEPTH", "NAV_HEADING", "NAV_YAW", "NAV_SPEED", "NAV_PITCH",
            "NAV_ROLL", "NAV_LAT", "NAV_LONG", "LAT_ORIGIN", "LONG_ORIGIN", "VEHICLE_NAME",
            "SONAR_CONTROL", "TGT_STATE_IN"],
    outputs=["SONAR_DIR", "SONAR_FILE_NUM", "SONAR_FILENAME"])


class MitBluefin21(Bluefin21):
    components = Bluefin21.components + [UdpAcomms, PassiveSonar]
    subsystems = Bluefin21.subsystems + [
        Subsystem("sensor_sim", [Interprocess, Health, Logging, Targets, PassiveSonarSim, SENSOR_SIM_LINK],
                  mode="sim")]
