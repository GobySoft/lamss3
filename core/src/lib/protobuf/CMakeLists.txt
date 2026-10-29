# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_add_proto(protobuf
  PROTOS
    lamss/core/environment_update.proto
    lamss/core/header.proto
    lamss/core/node_status.proto
    lamss/core/options.proto
    lamss/core/status.proto
    lamss/core/status_topside.proto
    lamss/core/tagged_acomms.proto
    lamss/core/view_circle.proto
    lamss/core/view_point.proto
    lamss/core/view_polygon.proto
    lamss/core/view_seglist.proto)

target_include_directories(lamss_core_protobuf PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
install(FILES lamss/core/groups.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lamss/core)
