# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""The smallest runnable mission: one topside with the core components, used by CI."""

from lamss.mission import Cruise


class Example(Cruise):
    fleet = "example_fleet"
    environment = "synthetic"
    params = dict(datum=dict(lat=42.358, lon=-71.087))
