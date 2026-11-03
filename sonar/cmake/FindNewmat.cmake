# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

# Finds Newmat 10 (Ubuntu's libnewmat10-dev) and defines Newmat::Newmat.
# Its headers declare everything in namespace NEWMAT.

find_path(Newmat_INCLUDE_DIR newmat/newmat.h)
find_library(Newmat_LIBRARY newmat)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Newmat REQUIRED_VARS Newmat_LIBRARY Newmat_INCLUDE_DIR)

if(Newmat_FOUND AND NOT TARGET Newmat::Newmat)
  add_library(Newmat::Newmat UNKNOWN IMPORTED)
  set_target_properties(Newmat::Newmat PROPERTIES
    IMPORTED_LOCATION "${Newmat_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Newmat_INCLUDE_DIR}")
endif()
mark_as_advanced(Newmat_INCLUDE_DIR Newmat_LIBRARY)
