# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_moos()
lamss_find_ivp(mbutil)
lamss_add_library(henrik_util
  SOURCES
    CBessel.cpp
    CCovMat.cpp
    CEigenRays.cpp
    CMeasAvg.cpp
    CNoiseCovariance.cpp
    CRapRays.cpp
    CTarget.cpp
    CVectorUtils.cpp
    Measurement.cpp
    Random.cpp
    parse_stat.cpp
    str_num.cpp
  LINK_LIBRARIES MOOS::MOOS IvP::mbutil)
