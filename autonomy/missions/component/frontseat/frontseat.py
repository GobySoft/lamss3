# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""The backseat's link to the vehicle's own computer (the frontseat)."""

from lamss.mission import Component

from missions.component.interprocess import Interprocess


class Frontseat(Component):
    requires = [Interprocess]
