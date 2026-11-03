# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_moos()
lamss_find_ivp(mbutil)
lamss_add_library(henrik_util
  SOURCES
    CMeasAvg.cpp
    CVectorUtils.cpp
    Measurement.cpp
    parse_stat.cpp
  LINK_LIBRARIES MOOS::MOOS IvP::mbutil)
