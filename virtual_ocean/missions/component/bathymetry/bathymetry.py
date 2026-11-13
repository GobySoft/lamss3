# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""iNoaaBathy: the water depth under the vehicle, from the environment's NetCDF bathymetry grid
(`environment.bathymetry.file`, relative to `environment.data_dir`)."""

from lamss.mission import Component, Process

from missions.component.interprocess import Interprocess


class Bathymetry(Component):
    requires = [Interprocess]
    sim_only = True
    params = dict(publish_interval=5)
    processes = [Process("iNoaaBathy", "iNoaaBathy last.moos")]
    fragments = {"last.moos": ["iNoaaBathy.moos.j2"]}
