# Copyright 2017-2026 Massachusetts Institute of Technology
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_ivp(geometry mbutil)
lamss_add_app(pTetraHoming MOOS NO_GOBY
  SOURCES main.cpp TetraHoming.cpp TetraHoming_Info.cpp
  LINK_LIBRARIES IvP::geometry IvP::mbutil)
