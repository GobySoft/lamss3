# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

from lamss.mission import Component, Process

from missions.component.moos import Moos


class App(Component):
    requires = [Moos]
    params = dict(rate=4, label="100% app")
    fragments = {"last.moos": ["app.moos.j2"]}
    processes = [Process("app", "sleep 302", autorestart=True)]
