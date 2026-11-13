# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Unicorn, an MIT Bluefin 21, Macrura's sister vehicle."""

from missions.platform.auv.propellor.bluefin21.mit_bluefin21 import MitBluefin21


class Unicorn(MitBluefin21):
    params = dict(modem_id=3)
