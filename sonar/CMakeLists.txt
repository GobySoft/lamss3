# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22...4.2)
project(lamss_sonar C CXX)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/LamssToolbox.cmake OPTIONAL RESULT_VARIABLE _in_tree)
if(NOT _in_tree)
  find_package(lamss_core REQUIRED)
  include(LamssToolbox)
endif()
lamss_toolbox(NAME sonar REQUIRES core autonomy)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(src/lib/protobuf)
add_subdirectory(src/lib/henrik_util)
add_subdirectory(src/lib/track)
add_subdirectory(src/bin/pGeoTracker)
add_subdirectory(src/bin/pLamssAEL)
add_subdirectory(src/bin/pLamssBearingTracker)
add_subdirectory(src/bin/pLamssCBF)
if(BUILD_TESTING)
  add_subdirectory(src/test)
endif()

lamss_toolbox_export()
