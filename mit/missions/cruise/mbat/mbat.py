# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Massachusetts Bay AUV Testbed (MBAT), centred on the dumping area east of Boston."""

from lamss.mission import Cruise


class Mbat(Cruise):
    fleet = "mit_bf21"
    environment = "mass_bay"
    params = dict(datum=dict(lat=42.40587, lon=-70.5619),
                  op_box=[[-4000, -4000], [-4000, 4000], [4000, 4000], [4000, -4000]],
                  gateway_buoy=dict(x=0, y=-800))
