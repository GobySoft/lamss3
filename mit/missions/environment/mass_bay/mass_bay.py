# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Massachusetts Bay around the MBAT dumping area: about 100 m deep, downward-refracting winter
profile."""

from lamss.mission import Environment


class MassBay(Environment):
    params = dict(bathymetry=dict(kind="flat", depth=100.0),
                  sound_speed=dict(surface=1480.0, gradient=0.001), currents=None, ice=None)
