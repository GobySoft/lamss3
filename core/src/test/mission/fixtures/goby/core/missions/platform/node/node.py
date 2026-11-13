# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

from lamss.mission import Platform

from missions.component.gobyd import Gobyd


class Node(Platform):
    components = [Gobyd]
