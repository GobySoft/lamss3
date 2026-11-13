# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Goby3's iFrontSeat_bluefin, speaking the Bluefin Huxley protocol to the vehicle computer, or in
simulation to iBackSeatServer in the platform's frontseat_sim subsystem."""

from lamss.mission import MissionError, Process

from missions.component.frontseat import Frontseat


class BluefinFrontseat(Frontseat):
    params = dict(huxley_address="localhost", huxley_port=None)
    processes = [Process("iFrontSeat_bluefin", "iFrontSeat_bluefin last.moos")]
    fragments = {"last.moos": ["iFrontSeat_bluefin.moos.j2"]}

    def derived(self):
        port = self.params["huxley_port"]
        if port is None:
            sim = next((s for s in self.platform.resolved_subsystems if s.name == "frontseat_sim"), None)
            if sim is None or "backseat" not in sim.allocated_ports:
                raise MissionError(f"{self.path}: set huxley_port (no frontseat_sim serves one)")
            port = sim.allocated_ports["backseat"]
        return {"huxley_port": port}
