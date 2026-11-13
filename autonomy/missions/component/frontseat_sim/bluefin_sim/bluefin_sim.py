# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""A Bluefin 21: uMVS_Bluefin dynamics, pMarinePID as its autopilot, and iBackSeatServer serving the
Huxley protocol to iFrontSeat_bluefin on the platform's main subsystem. pEchoVar gives pMarinePID
uMVS_Bluefin's FRONTSEAT_* state as NAV_*."""

from lamss.mission import Process

from missions.component.frontseat_sim import FrontseatSim


class BluefinFrontseatSim(FrontseatSim):
    params = dict(launch=dict(x=0, y=0, depth=0, heading=15))
    ports = ["backseat"]
    processes = [Process("uMVS_Bluefin", "uMVS_Bluefin last.moos"),
                 Process("pMarinePID", "pMarinePID last.moos"),
                 Process("iBackSeatServer", "iBackSeatServer last.moos"),
                 Process("pEchoVar", "pEchoVar last.moos")]
    fragments = {"last.moos": ["uMVS_Bluefin.moos.j2", "pMarinePID.moos.j2", "iBackSeatServer.moos.j2",
                               "pEchoVar.moos.j2"]}
