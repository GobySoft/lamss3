# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

from lamss.mission import Component, Process


class Moos(Component):
    ports = ["moosdb"]
    templates = ["last.moos.j2"]
    processes = [Process("moosdb", "sleep 301", startsecs=0)]
