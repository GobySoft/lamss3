# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_ivp(mbutil)
lamss_add_app(pGeoTracker MOOS NO_GOBY
  SOURCES pGeoTrackMain.cpp GeoTracker.cpp GeoTrack.cpp
  LINK_LIBRARIES lamss::sonar_track lamss::core_henrik_util IvP::mbutil)
