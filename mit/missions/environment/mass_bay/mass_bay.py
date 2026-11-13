# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Massachusetts Bay: the Gulf of Maine bathymetry grid and a downward-refracting winter profile.
Its simulators join each platform's frontseat_sim subsystem."""

from pathlib import Path

from lamss.mission import Environment

from missions.component.bathymetry import Bathymetry


class MassBay(Environment):
    params = dict(bathymetry=dict(kind="netcdf", file="gulf_of_maine_bathymetry.nc", depth=100.0),
                  sound_speed=dict(surface=1480.0, gradient=0.001), currents=None, ice=None)
    components = [Bathymetry]
    subsystem = "frontseat_sim"

    def derived(self):
        return {"data_dir": str(Path(__file__).parent / "data")}
