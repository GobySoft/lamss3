# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""The vehicle's sonar processing, from array data to tracks."""

from lamss.mission import Component

from missions.component.interprocess import Interprocess


class Sonar(Component):
    requires = [Interprocess, "mission_manager"]
