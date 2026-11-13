# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""The MIT Bluefin 21 fleet, commanded from R/V Resolution."""

from lamss.mission import Fleet

from missions.platform.auv.propellor.bluefin21.mit_bluefin21.macrura import Macrura
from missions.platform.topside.resolution import Resolution


class MitBf21(Fleet):
    platforms = [Resolution, Macrura(topside="resolution")]
