# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

from lamss.mission import Subsystem

from missions.component.daemon import Daemon
from missions.platform.station import Station


class Buoy(Station):
    params = dict(type="buoy")
    subsystems = Station.subsystems + [Subsystem("sensors", [Daemon(seconds=303)], mode="sim")]
