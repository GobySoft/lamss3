# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""A towed array and what it hears: uSimTowedArray (array shape behind the vehicle),
uSimNoiseCovariance (ambient noise) and uSimPassiveSonar (element time series of the simulated
targets). The data files go in the subsystem's run directory and are announced by SONAR_FILENAME
for a beamformer to read. Propagation is analytic ray tracing in the environment's linear profile,
or with `bellhop` iBellhop's eigenrays (not run by this component). The defaults are MIT's DURIP
array."""

from lamss.mission import Component, Process

from missions.component.interprocess import Interprocess


class PassiveSonarSim(Component):
    requires = [Interprocess]
    sim_only = True
    # data files (3 MB a frame) older than keep_minutes are deleted
    params = dict(bellhop=False, keep_minutes=2, noise_level=40, elements=32, sampling_frequency=12000, samples=24000,
                  array_type="DURIP",
                  tow_point=dict(x=0.0, y=-2.1, z=-0.25),
                  spacing_groups=[dict(spacing=1.5, count=5), dict(spacing=0.75, count=22),
                                  dict(spacing=1.5, count=4)])
    processes = [Process("uSimTowedArray", "uSimTowedArray last.moos"),
                 Process("uSimNoiseCovariance", "uSimNoiseCovariance last.moos"),
                 Process("uSimPassiveSonar", "uSimPassiveSonar last.moos"),
                 Process("sonar_data_cleaner", "sh sonar_data_cleaner.sh", "sonar_data_cleaner.sh.j2")]
    fragments = {"last.moos": ["uSimTowedArray.moos.j2", "uSimNoiseCovariance.moos.j2",
                               "uSimPassiveSonar.moos.j2"]}
