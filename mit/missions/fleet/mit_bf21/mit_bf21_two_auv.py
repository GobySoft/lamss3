# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""The fleet with Unicorn as well, launched and loitering 600 m east of Macrura."""

from lamss.mission import variant

from missions.fleet.mit_bf21 import MitBf21
from missions.platform.auv.propellor.bluefin21.mit_bluefin21.unicorn import Unicorn


@variant("two_auv")
class MitBf21TwoAuv(MitBf21):
    platforms = MitBf21.platforms + [Unicorn(topside="resolution")]
    overrides = {"platform.unicorn.subsystem.frontseat_sim.component.frontseat_sim.launch.x": 600,
                 "platform.unicorn.subsystem.main.component.mission_manager.initial_deploy.x": 600}
