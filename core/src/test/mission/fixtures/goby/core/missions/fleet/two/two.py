# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

from lamss.mission import Fleet

from missions.platform.node import Node


class Two(Fleet):
    platforms = [Node("alpha"), Node("beta")]
