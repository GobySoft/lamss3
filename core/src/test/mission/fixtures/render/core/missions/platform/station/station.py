# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

from lamss.mission import Platform

from missions.component.app import App
from missions.component.daemon import Daemon
from missions.component.moos import Moos


class Station(Platform):
    params = dict(type="station")
    components = [Daemon, Moos, App]
