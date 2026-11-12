# Copyright 2011-2026 Massachusetts Institute of Technology
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_find_ivp(mbutil)
lamss_add_app(pGateway MOOS SOURCES pGatewaymain.cpp Gateway.cpp LINK_LIBRARIES IvP::mbutil)
