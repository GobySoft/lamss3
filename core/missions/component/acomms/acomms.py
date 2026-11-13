# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""pAcommsHandler, sending the cruise's acomms messages (`cruise.acomms.messages`) in the cruise's MAC
slots (`cruise.acomms.slots`). Each implementation provides its driver in `acomms_driver.pb.cfg.j2`."""

from lamss.mission import Component, MissionError, Process

from missions.component.interprocess import Interprocess


class Acomms(Component):
    requires = [Interprocess]
    processes = [Process("pAcommsHandler", "pAcommsHandler acomms.moos", "acomms.moos.j2")]
    templates = ["modemidlookup.txt.j2"]

    def derived(self):
        if self.platform.params.get("modem_id") is None:
            raise MissionError(f"{self.path}: the platform needs a modem_id")
        return {}
