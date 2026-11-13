# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""goby_logger, writing every interprocess message to the subsystem's log directory."""

from lamss.mission import Component, Process

from missions.component.interprocess import Interprocess


class Logging(Component):
    requires = [Interprocess]
    params = dict(group_regex=".*", type_regex=".*")
    processes = [Process("goby_logger", "goby_logger goby_logger.pb.cfg", "goby_logger.pb.cfg.j2",
                         health=True)]
