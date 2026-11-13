# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""One gobyd per subsystem and, until the MOOS apps are rewritten, a MOOSDB bridged to it by
lamss_goby_moos_gateway with the lamss plugin. MOOS app components render their ProcessConfig
into last.moos through `fragments`."""

from pathlib import Path

from lamss.mission import Component, MissionError, Process
from lamss.mission.render import socket_path


class Interprocess(Component):
    params = dict(moos=True, verbosity="WARN")
    ports = ["moosdb"]

    def active_ports(self):
        return self.ports if self.params["moos"] else []

    def active_processes(self):
        processes = [Process("gobyd", "gobyd gobyd.pb.cfg", "gobyd.pb.cfg.j2", health=True)]
        if self.params["moos"]:
            processes += [
                Process("MOOSDB", "MOOSDB last.moos"),
                Process("goby_moos_gateway", "lamss_goby_moos_gateway goby_moos_gateway.pb.cfg",
                        "goby_moos_gateway.pb.cfg.j2", health=True),
            ]
        return processes

    def active_templates(self):
        return ["last.moos.j2"] if self.params["moos"] else []

    def derived(self):
        if self.params["verbosity"] not in ("QUIET", "WARN", "VERBOSE", "DEBUG1", "DEBUG2", "DEBUG3"):
            raise MissionError(f"{self.path}: verbosity must be a goby glog verbosity, "
                               f"not {self.params['verbosity']!r}")
        main = self.subsystem.name == self.platform.main_subsystem
        return {"community": self.platform.name if main else f"{self.platform.name}_{self.subsystem.name}"}

    def settings(self, paths):
        return {"platform": self.platform.name,
                "socket_name": str(socket_path(Path(paths["run"]), "gobyd"))}

    def environment(self, paths):
        s = self.settings(paths)
        return {"GOBY_INTERPROCESS": f'platform: "{s["platform"]}" socket_name: "{s["socket_name"]}"'}

    def test_endpoint(self, paths):
        return {"zeromq": self.settings(paths)}
