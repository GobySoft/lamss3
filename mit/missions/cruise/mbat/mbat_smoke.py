# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""The mbat cruise twice as fast as real time, for CI."""

from lamss.mission import variant

from missions.cruise.mbat import Mbat


@variant("smoke")
class MbatSmoke(Mbat):
    # on CI's four shared cores uSimPassiveSonar keeps up at warp 2 but not at 5
    params = dict(warp=2)
