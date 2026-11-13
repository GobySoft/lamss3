# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Massachusetts Bay AUV Testbed (MBAT), centred on the dumping area east of Boston."""

from lamss.mission import Cruise


class Mbat(Cruise):
    fleet = "mit_bf21"
    environment = "mass_bay"
    params = dict(datum=dict(lat=42.40587, lon=-70.5619),
                  op_box=[[-4000, -4000], [-4000, 4000], [4000, 4000], [4000, -4000]],
                  gateway_buoy=dict(x=0, y=-800),
                  target=dict(x=1500, y=1000, depth=5, heading=280, speed=2, freq=1000, bandwidth=1000,
                              spl=140, strength=10),
                  # goby_liaison's commander queues these for pAcommsHandler through the gateway
                  commands=[dict(type="lamss.autonomy.LAMSS_DEPLOY", group="lamss::goby::pAcommsHandler_multiplex")],
                  acomms=dict(messages=["lamss_status", "lamss_deploy", "lamss_track"],
                              slots=[dict(src=1, seconds=12), dict(src=4, dest=1, rate=1, seconds=12)]))
