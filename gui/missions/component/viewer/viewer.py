# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""goby_liaison and, while the MOOS apps run, pMarineViewer when it is installed and there is a
display to show it on (or `marine_viewer: true`)."""

import os
import shutil

from lamss.mission import Component, MissionError, Process

from missions.component.interprocess import Interprocess


class Viewer(Component):
    requires = [Interprocess]
    params = dict(http_address="localhost", marine_viewer="auto")
    ports = {"liaison": "http"}

    def marine_viewer(self):
        setting = self.params["marine_viewer"]
        if setting not in ("auto", True, False):
            raise MissionError(f"{self.path}: marine_viewer must be auto, true or false")
        moos = next(c for c in self.subsystem.components if c.name == "interprocess").params["moos"]
        if setting == "auto":
            display = os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY")
            return moos and bool(display) and shutil.which("pMarineViewer") is not None
        return setting and moos

    def active_processes(self):
        processes = [Process("goby_liaison", "goby_liaison liaison.pb.cfg", "liaison.pb.cfg.j2", health=True)]
        if self.marine_viewer():
            processes.append(Process("pMarineViewer", "pMarineViewer last.moos"))
        return processes

    def active_fragments(self):
        return {"last.moos": ["pMarineViewer.moos.j2"]} if self.marine_viewer() else {}
