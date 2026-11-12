# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_add_dccl_test(lamss_virtual_ocean_protobuf_dccl SOURCES dccl_load.cpp
  LINK_LIBRARIES lamss::core_protobuf lamss::virtual_ocean_protobuf)
