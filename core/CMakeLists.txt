# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22...4.2)
project(lamss_core CXX)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/LamssToolbox.cmake)
lamss_toolbox(NAME core)

install(FILES ${LAMSS_CMAKE_DIR}/LamssToolbox.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lamss_core)

lamss_toolbox_export()
