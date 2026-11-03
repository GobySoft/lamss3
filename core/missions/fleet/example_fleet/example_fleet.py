# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
from lamss.mission import Fleet

from missions.platform.topside import Topside


class ExampleFleet(Fleet):
    platforms = [Topside]
