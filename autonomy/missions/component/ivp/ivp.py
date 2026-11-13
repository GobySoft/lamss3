# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""pHelmIvP with the LAMSS behavior set (modes driven by pLamssMissionManager), and
pBasicContactMgr for the behaviors that act on contacts."""

from lamss.mission import Component, Process

from missions.component.interprocess import Interprocess


class Ivp(Component):
    requires = [Interprocess, "mission_manager"]
    processes = [Process("pBasicContactMgr", "pBasicContactMgr last.moos"),
                 Process("pHelmIvP", "pHelmIvP last.moos")]
    templates = ["lamss.bhv.j2"]
    fragments = {"last.moos": ["pBasicContactMgr.moos.j2", "pHelmIvP.moos.j2"]}
