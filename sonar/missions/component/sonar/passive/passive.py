# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""The passive chain: pLamssAEL (array position from NAV_DATA), pLamssCBF (beamforming the files a
recorder or simulator announces in SONAR_FILENAME), pLamssBearingTracker, pGeoTracker and
pLamssTrackManager, whose TRACK_REPORT goes to the topside. The defaults are MIT's DURIP towed
array as used on macrura."""

from lamss.mission import Process

from missions.component.sonar import Sonar


class PassiveSonar(Sonar):
    params = dict(array_center_offset=-38.8,
                  spacing_groups=[dict(spacing=1.5, count=5), dict(spacing=0.75, count=22),
                                  dict(spacing=1.5, count=4)],
                  elements_used="6:28", beams=47, center_frequency=800, bandwidth=400,
                  frequency_bins="205:258", sampling_frequency=12000, samples=24000)
    processes = [Process("pLamssAEL", "pLamssAEL last.moos"),
                 Process("pLamssCBF", "pLamssCBF last.moos"),
                 Process("pLamssBearingTracker", "pLamssBearingTracker last.moos"),
                 Process("pGeoTracker", "pGeoTracker last.moos"),
                 Process("pLamssTrackManager", "pLamssTrackManager last.moos")]
    fragments = {"last.moos": ["passive_sonar.moos.j2"]}
