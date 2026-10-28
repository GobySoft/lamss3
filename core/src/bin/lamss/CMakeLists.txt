# Copyright 2026 Massachusetts Institute of Technology
# SPDX-License-Identifier: GPL-3.0-or-later

if(NOT TARGET goby)
  find_package(goby 3.6 REQUIRED)
endif()

add_library(lamss_tool_common STATIC repo.cpp process.cpp)
target_include_directories(lamss_tool_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lamss_tool_common PUBLIC goby)

lamss_add_app(lamss
  SOURCES
    lamss.cpp
    cmd/build.cpp
    cmd/clean.cpp
    cmd/env.cpp
    cmd/toolbox.cpp
  PROTOS lamss.proto
  LINK_LIBRARIES lamss_tool_common)

# init.sh installs only this component, from a core-only build tree
install(TARGETS lamss RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT lamss_tool EXCLUDE_FROM_ALL)
