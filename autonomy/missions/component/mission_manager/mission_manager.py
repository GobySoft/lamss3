# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""pLamssMissionManager: the vehicle's mission state machine, started on the deploy command the
platform's `initial_deploy` describes."""

from lamss.mission import Component, MissionError, Process

from missions.component.interprocess import Interprocess


class MissionManager(Component):
    requires = [Interprocess]
    params = dict(
        operation_radius=5000, min_opbox_distance=100, deploy_duration=4800,
        prosecute_duration=3600, abort=dict(x_offset=50, y_offset=50, depth=5),
        initial_deploy=dict(mode="LOITER", depth_mode="SINGLE", x=0, y=0, speed=1.8,
                            upper_depth=15, lower_depth=50, altitude=6, loiter_radius=200,
                            loiter_segments=6, survey_heading=120, survey_length=3000,
                            survey_width=200, survey_period=300, clockwise=False),
        initial_transit=dict(mode="DIRECT", depth=10, speed=1.6))
    processes = [Process("pLamssMissionManager", "pLamssMissionManager last.moos")]
    fragments = {"last.moos": ["pLamssMissionManager.moos.j2"]}

    def derived(self):
        topside = self.platform.params.get("topside")
        if not topside:
            raise MissionError(f"{self.path}: the platform needs a topside to take commands from")
        return {"topside": topside}
