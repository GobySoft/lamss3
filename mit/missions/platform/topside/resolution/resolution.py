# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""R/V Resolution, the command and control ship of the MIT Bluefin 21 cruises."""

from missions.component.acomms.udp import UdpAcomms
from missions.platform.topside import Topside


class Resolution(Topside):
    params = dict(modem_id=1)
    components = Topside.components + [UdpAcomms]
