# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

from lamss.mission import variant

from missions.cruise.harbor import Harbor


@variant("quick")
class HarborQuick(Harbor):
    params = dict(warp=5)
