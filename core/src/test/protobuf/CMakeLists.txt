# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_add_test(lamss_core_protobuf_dccl SOURCES dccl_load.cpp LINK_LIBRARIES lamss::core_protobuf)
target_compile_definitions(lamss_core_protobuf_dccl
  PRIVATE LAMSS_DCCL_IDS_MD="${PROJECT_SOURCE_DIR}/src/lib/protobuf/lamss/DCCL_IDS.md")
