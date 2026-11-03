# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""goby_coroner, which expects every process of its subsystem that answers health requests."""

from lamss.mission import Component, Process

from missions.component.interprocess import Interprocess


class Health(Component):
    requires = [Interprocess]
    params = dict(request_interval=5, response_timeout=3)
    processes = [Process("goby_coroner", "goby_coroner goby_coroner.pb.cfg", "goby_coroner.pb.cfg.j2",
                         health=True)]

    def derived(self):
        return {"expected": [p.name for c in self.subsystem.components for p in c.active_processes()
                             if p.health]}
