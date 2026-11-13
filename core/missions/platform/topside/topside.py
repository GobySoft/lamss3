# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
from lamss.mission import Platform, optional

from missions.component.health import Health
from missions.component.interprocess import Interprocess
from missions.component.logging import Logging
from missions.component.view_message import ViewMessage


class Topside(Platform):
    params = dict(type="topside")
    components = [Interprocess, Health, Logging, ViewMessage, optional("viewer")]
