# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22...4.2)
project(lamss_autonomy CXX)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/LamssToolbox.cmake OPTIONAL RESULT_VARIABLE _in_tree)
if(NOT _in_tree)
  find_package(lamss_core REQUIRED)
  include(LamssToolbox)
endif()
lamss_toolbox(NAME autonomy REQUIRES core)

add_subdirectory(src/lib/protobuf)
add_subdirectory(src/bin/pGateway)
add_subdirectory(src/bin/pLamssMissionManager)
add_subdirectory(src/bin/pTetraHoming)
add_subdirectory(src/bin/uMVS_Bluefin)
if(BUILD_TESTING)
  add_subdirectory(src/test)
endif()

lamss_toolbox_export()
