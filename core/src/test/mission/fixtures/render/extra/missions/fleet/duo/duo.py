# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

from lamss.mission import Fleet

from missions.platform.station import Station
from missions.platform.station.buoy import Buoy


class Duo(Fleet):
    platforms = [Station("shore", modem_id=1), Buoy(modem_id=2)]
