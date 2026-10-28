# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22...4.2)
project(lamss3 NONE)

enable_testing()
include(${CMAKE_SOURCE_DIR}/cmake/LamssToolbox.cmake)

file(STRINGS "${CMAKE_SOURCE_DIR}/.lamss/toolboxes" LAMSS_TOOLBOXES REGEX "^[^#]")
list(TRANSFORM LAMSS_TOOLBOXES STRIP)
list(FILTER LAMSS_TOOLBOXES EXCLUDE REGEX "^$")
list(PREPEND LAMSS_TOOLBOXES core)
list(REMOVE_DUPLICATES LAMSS_TOOLBOXES)
set_property(GLOBAL PROPERTY LAMSS_ACTIVE_TOOLBOXES ${LAMSS_TOOLBOXES})
# re-run configure when the activation list changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/.lamss/toolboxes")

foreach(tb IN LISTS LAMSS_TOOLBOXES)
  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${tb}/CMakeLists.txt")
    message(FATAL_ERROR "toolbox '${tb}' is active but missing; run `lamss toolbox add ${tb}`")
  endif()
  add_subdirectory(${tb})
endforeach()

message(STATUS "lamss3 active toolboxes: ${LAMSS_TOOLBOXES}")
