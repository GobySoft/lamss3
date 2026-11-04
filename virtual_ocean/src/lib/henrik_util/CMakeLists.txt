# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_add_library(henrik_util
  SOURCES CEigenRays.cpp CLamssNetcdf.cpp CRapRays.cpp
  LINK_LIBRARIES lamss::core_henrik_util goby_moos netCDF::netcdf Boost::filesystem)
