# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""pHelmIvP with the LAMSS behavior set (modes driven by pLamssMissionManager), and
pBasicContactMgr for the behaviors that act on contacts."""

import re
import shutil
import subprocess

from lamss.mission import Component, MissionError, Process

from missions.component.interprocess import Interprocess


class Ivp(Component):
    requires = [Interprocess, "mission_manager"]
    # "auto" asks the pHelmIvP on PATH when the mission is generated
    params = dict(moos_ivp_version="auto")
    processes = [Process("pBasicContactMgr", "pBasicContactMgr last.moos"),
                 Process("pHelmIvP", "pHelmIvP last.moos")]
    templates = ["lamss.bhv.j2"]
    fragments = {"last.moos": ["pBasicContactMgr.moos.j2", "pHelmIvP.moos.j2"]}

    def derived(self):
        version = self.params["moos_ivp_version"]
        if version == "auto":
            version = installed_moos_ivp_version()
        try:
            major = int(str(version).split(".")[0])
        except ValueError:
            raise MissionError(f"{self.path}: moos_ivp_version must be auto or a version such as 24.8, "
                               f"not {version!r}") from None
        # MOOS-IvP 24.8 replaced BHV_AvoidObstacle with BHV_AvoidObstacleV24
        return {"avoid_obstacle": "BHV_AvoidObstacleV24" if major >= 24 else "BHV_AvoidObstacle"}


def installed_moos_ivp_version(default="24.8"):
    helm = shutil.which("pHelmIvP")
    if helm is None:
        return default
    try:
        out = subprocess.run([helm, "--version"], capture_output=True, text=True, timeout=10).stdout
    except (OSError, subprocess.TimeoutExpired):
        return default
    match = re.search(r"Version\s+(\d+(?:\.\d+)*)", out)
    return match.group(1) if match else default
