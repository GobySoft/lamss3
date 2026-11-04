# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""pViewMessage: view objects between MOOS and the lamss.core view messages."""

from lamss.mission import Component, MissionError, Process

from missions.component.interprocess import Interprocess


class ViewMessage(Component):
    requires = [Interprocess]
    params = dict(generate_view_objects=True, destination=1)
    processes = [Process("pViewMessage", "pViewMessage last.moos")]
    fragments = {"last.moos": ["pViewMessage.moos.j2"]}

    def derived(self):
        if not self.subsystem_component("interprocess").params["moos"]:
            raise MissionError(f"{self.path} is a MOOS app: needs interprocess.moos")
        return {}

    def subsystem_component(self, name):
        return next(c for c in self.subsystem.components if c.name == name)
