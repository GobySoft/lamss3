# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

from lamss.mission import Component, Process
from lamss.mission.render import socket_path


class Gobyd(Component):
    processes = [Process("gobyd", "gobyd gobyd.pb.cfg", "gobyd.pb.cfg.j2")]

    def interprocess(self, paths):
        return {"platform": self.platform.name,
                "socket_name": str(socket_path(__import__("pathlib").Path(paths["run"]), "gobyd"))}

    def environment(self, paths):
        settings = self.interprocess(paths)
        return {"GOBY_INTERPROCESS": f'platform: "{settings["platform"]}" socket_name: "{settings["socket_name"]}"'}

    def test_endpoint(self, paths):
        return {"zeromq": self.interprocess(paths)}
