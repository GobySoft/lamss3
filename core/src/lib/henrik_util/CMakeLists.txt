# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_moos()
lamss_find_ivp(mbutil)
lamss_add_library(henrik_util
  SOURCES
    CEigenRays.cpp
    CMeasAvg.cpp
    CRapRays.cpp
    CVectorUtils.cpp
    Measurement.cpp
    parse_stat.cpp)
# not PUBLIC: no header includes MOOS or MOOS-IvP, and lamss_coreConfig.cmake does not define them
target_link_libraries(lamss_core_henrik_util PRIVATE MOOS::MOOS IvP::mbutil)
