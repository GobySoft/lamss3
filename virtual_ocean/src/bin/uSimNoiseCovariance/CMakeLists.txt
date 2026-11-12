# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(GSL REQUIRED)
lamss_find_ivp(geometry)
lamss_add_app(uSimNoiseCovariance MOOS
  SOURCES SimNoiseCovariance.cpp SimNoiseCovarianceMain.cpp
  LINK_LIBRARIES lamss::core_henrik_util IvP::mbutil IvP::geometry GSL::gsl)
