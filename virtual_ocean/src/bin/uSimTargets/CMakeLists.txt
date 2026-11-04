# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_add_app(uSimTargets MOOS
  SOURCES SimTargets.cpp
  PROTOS SimTargets_config.proto
  LINK_LIBRARIES lamss::virtual_ocean_protobuf lamss::core_henrik_util IvP::mbutil goby_frontseat_bluefin)
