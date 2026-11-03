# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

from lamss.mission import Component, Process


class Daemon(Component):
    params = dict(seconds=300)
    ports = ["daemon"]
    processes = [Process("daemon", "sleep {{ component.daemon.seconds }}", "daemon.cfg.j2")]

    def environment(self):
        return {"DAEMON_PORT": str(self.subsystem.allocated_ports["daemon"])}
