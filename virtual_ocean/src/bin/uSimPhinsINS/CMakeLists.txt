# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_add_app(uSimPhinsINS MOOS
  SOURCES SimPhinsINS.cpp SimPhinsINS_Info.cpp main.cpp
  LINK_LIBRARIES lamss::core_protobuf lamss::core_henrik_util IvP::mbutil)
