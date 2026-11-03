# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

from lamss.mission import Cruise


class Harbor(Cruise):
    fleet = "duo"
    params = dict(datum=dict(lat=42.358, lon=-71.087), port_base=24000)
