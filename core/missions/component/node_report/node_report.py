# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""MOOS-IvP's pNodeReporter, posting the platform's NODE_REPORT from NAV_* for pMarineViewer, the
contact managers of other platforms and, through the lamss gateway plugin, Goby."""

from lamss.mission import Component, Process

from missions.component.interprocess import Interprocess


class NodeReport(Component):
    requires = [Interprocess]
    processes = [Process("pNodeReporter", "pNodeReporter last.moos")]
    fragments = {"last.moos": ["pNodeReporter.moos.j2"]}
