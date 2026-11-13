# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""The mbat cruise ten times faster than real time, for CI."""

from lamss.mission import variant

from missions.cruise.mbat import Mbat


@variant("smoke")
class MbatSmoke(Mbat):
    params = dict(warp=10)
