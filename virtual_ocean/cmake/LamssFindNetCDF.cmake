# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

# Defines netCDF::netcdf (the C library) from netCDF's own package or, without one, directly.
find_package(netCDF CONFIG QUIET)
if(NOT TARGET netCDF::netcdf)
  find_library(LAMSS_NETCDF_LIBRARY netcdf REQUIRED)
  find_path(LAMSS_NETCDF_INCLUDE_DIR netcdf.h REQUIRED)
  add_library(netCDF::netcdf UNKNOWN IMPORTED)
  set_target_properties(netCDF::netcdf PROPERTIES
    IMPORTED_LOCATION "${LAMSS_NETCDF_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${LAMSS_NETCDF_INCLUDE_DIR}")
endif()
