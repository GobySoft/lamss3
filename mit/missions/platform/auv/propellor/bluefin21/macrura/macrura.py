# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Macrura, MIT's Bluefin 21."""

from missions.component.acomms.udp import UdpAcomms
from missions.platform.auv.propellor.bluefin21 import Bluefin21


class Macrura(Bluefin21):
    params = dict(modem_id=4)
    components = Bluefin21.components + [UdpAcomms]
