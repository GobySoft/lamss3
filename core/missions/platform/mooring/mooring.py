# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
from lamss.mission import Platform

from missions.component.health import Health
from missions.component.interprocess import Interprocess
from missions.component.logging import Logging


class Mooring(Platform):
    params = dict(type="mooring")
    components = [Interprocess, Health, Logging]
