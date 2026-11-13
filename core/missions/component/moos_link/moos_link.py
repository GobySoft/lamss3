# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""pMOOSLink: copies MOOS variables between this subsystem's MOOSDB and another subsystem's of the
same platform (`remote`). `inputs` are pulled from the remote, `outputs` pushed to it; an entry
`LOCAL:REMOTE` renames."""

from lamss.mission import Component, MissionError, Process

from missions.component.interprocess import Interprocess


class MoosLink(Component):
    requires = [Interprocess]
    params = dict(remote="main", inputs=[], outputs=[])
    processes = [Process("pMOOSLink", "pMOOSLink last.moos")]
    fragments = {"last.moos": ["pMOOSLink.moos.j2"]}

    def derived(self):
        remote = next((s for s in self.platform.resolved_subsystems if s.name == self.params["remote"]), None)
        if remote is None or remote is self.subsystem:
            raise MissionError(f"{self.path}: remote must name another subsystem of {self.platform.name}")
        if "moosdb" not in remote.allocated_ports:
            raise MissionError(f"{self.path}: subsystem {remote.name} runs no MOOSDB")
        return {"remote_port": remote.allocated_ports["moosdb"]}
