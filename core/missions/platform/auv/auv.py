# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
from lamss.mission import Platform

from missions.component.health import Health
from missions.component.interprocess import Interprocess
from missions.component.logging import Logging
from missions.component.node_report import NodeReport
from missions.component.view_message import ViewMessage


class Auv(Platform):
    params = dict(type="auv", topside=None)
    components = [Interprocess, Health, Logging, ViewMessage, NodeReport]
