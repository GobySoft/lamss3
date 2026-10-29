# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22...4.2)
project(lamss_core CXX)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/LamssToolbox.cmake)
lamss_toolbox(NAME core)

install(FILES ${LAMSS_CMAKE_DIR}/LamssToolbox.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lamss_core)

add_subdirectory(src/lib/protobuf)
add_subdirectory(src/lib/henrik_util)
add_subdirectory(src/bin/iLineBasedComms)
add_subdirectory(src/bin/iSerialNMEA)
add_subdirectory(src/bin/lamss)
add_subdirectory(src/bin/pMOOSLink)
add_subdirectory(src/bin/pViewMessage)
if(BUILD_TESTING)
  add_subdirectory(src/test)
endif()

lamss_toolbox_export()
