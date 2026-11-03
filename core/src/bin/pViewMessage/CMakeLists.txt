# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_ivp(geometry mbutil)
lamss_add_app(pViewMessage MOOS
  SOURCES pViewMessageMain.cpp ViewMessage.cpp
  PROTOS view_message.proto
  LINK_LIBRARIES IvP::geometry IvP::mbutil)
