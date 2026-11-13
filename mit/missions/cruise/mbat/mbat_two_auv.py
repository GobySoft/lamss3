# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""The mbat cruise with Macrura and Unicorn, which gets its own acomms slot."""

from lamss.mission import variant

from missions.cruise.mbat import Mbat


@variant("two_auv")
class MbatTwoAuv(Mbat):
    overrides = {"cruise.acomms.slots": Mbat.params["acomms"]["slots"]
                 + [dict(src=3, dest=1, rate=1, seconds=12)]}
