# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

lamss_add_library(track SOURCES mat.c)
# K&R definitions, which C23 (GCC 15's default) no longer accepts
set_target_properties(lamss_sonar_track PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED ON)
