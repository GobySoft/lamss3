# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

# Also a project of its own: init.sh builds the tool from here before any Goby3 packages exist.
cmake_minimum_required(VERSION 3.22...4.2)
project(lamss_tool CXX)

if(PROJECT_IS_TOP_LEVEL)
  include(GNUInstallDirs)
  add_compile_options(-Wall -Wextra)
endif()

find_package(yaml-cpp REQUIRED)
# yaml-cpp 0.8 exports yaml-cpp::yaml-cpp; 0.7 (Ubuntu 22.04) only yaml-cpp
if(TARGET yaml-cpp::yaml-cpp)
  set(LAMSS_YAML_CPP yaml-cpp::yaml-cpp)
else()
  set(LAMSS_YAML_CPP yaml-cpp)
endif()

add_library(lamss_tool_common STATIC cli.cpp repo.cpp process.cpp deps.cpp)
target_compile_features(lamss_tool_common PUBLIC cxx_std_17)
target_include_directories(lamss_tool_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lamss_tool_common PUBLIC ${LAMSS_YAML_CPP})

add_executable(lamss
  lamss.cpp
  cmd/build.cpp
  cmd/clean.cpp
  cmd/dependencies.cpp
  cmd/env.cpp
  cmd/toolbox.cpp)
target_link_libraries(lamss PRIVATE lamss_tool_common)
install(TARGETS lamss RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(TARGET lamss_core_all)
  add_dependencies(lamss_core_all lamss)
endif()
