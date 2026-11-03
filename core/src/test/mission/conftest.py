# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

import textwrap
from pathlib import Path

import pytest

from lamss.mission import Registry, Toolbox

HEADER = "from lamss.mission import *\n"

# A small two-toolbox tree used by most tests: `extra` extends core's auv directory and holds the
# cruise, the way mit extends autonomy.
BASIC = {
    "core/missions/component/interprocess/interprocess.py": """
        class Interprocess(Component):
            params = dict(socket="gobyd", moos=dict(enabled=False, port=9000))
            processes = [Process("gobyd", "gobyd gobyd.pb.cfg", "gobyd.pb.cfg.j2")]
        """,
    "core/missions/component/logging/logging.py": """
        from missions.component.interprocess import Interprocess
        class Logging(Component):
            requires = [Interprocess]
            params = dict(level="INFO", dir="log")
        """,
    "core/missions/component/health/health.py": """
        class Health(Component):
            requires = ["interprocess"]
            params = dict(interval=1)
        """,
    "core/missions/component/acomms/acomms.py": """
        class Acomms(Component):
            params = dict(driver="none")
        """,
    "core/missions/component/acomms/udp/udp.py": """
        from missions.component.acomms import Acomms
        class Udp(Acomms):
            params = dict(driver="udp", port=11000)
        """,
    "core/missions/component/sim_only_thing/sim_only_thing.py": """
        class SimOnlyThing(Component):
            sim_only = True
        """,
    "core/missions/platform/auv/auv.py": """
        from missions.component.interprocess import Interprocess
        from missions.component.logging import Logging
        class Auv(Platform):
            params = dict(type="auv", max_depth=100, speed=1.5)
            components = [Interprocess, Logging]
        """,
    "core/missions/platform/topside/topside.py": """
        from missions.component.interprocess import Interprocess
        from missions.component.logging import Logging
        class Topside(Platform):
            params = dict(type="topside")
            components = [Interprocess, Logging(level="WARN"), optional("viewer")]
        """,
    "core/missions/environment/flat/flat.py": """
        class Flat(Environment):
            params = dict(depth=50)
        """,
    "extra/missions/platform/auv/glider/glider.py": """
        from missions.platform.auv import Auv
        from missions.component.health import Health
        from missions.component.sim_only_thing import SimOnlyThing
        class Glider(Auv):
            params = dict(max_depth=200, speed=0.3)
            components = Auv.components + [Health, SimOnlyThing]
        """,
    "extra/missions/platform/auv/glider/glider_deep.py": """
        from missions.platform.auv.glider import Glider
        @variant("deep")
        class GliderDeep(Glider):
            params = dict(max_depth=1000)
        """,
    "extra/missions/platform/auv/glider/glider_quiet.py": """
        from missions.platform.auv.glider import Glider
        from missions.component.health import Health
        @variant("quiet")
        class GliderQuiet(Glider):
            components = remove(Glider.components, Health)
        """,
    "extra/missions/platform/auv/glider/glider_acomms.py": """
        from missions.platform.auv.glider import Glider
        from missions.component.acomms.udp import Udp
        @variant("acomms")
        class GliderAcomms(Glider):
            components = Glider.components + [Udp(port=12000)]
        """,
    "extra/missions/platform/auv/glider/glider_deep_quiet.py": """
        from missions.platform.auv.glider.glider_deep import GliderDeep
        from missions.platform.auv.glider.glider_quiet import GliderQuiet
        from missions.platform.auv.glider.glider_acomms import GliderAcomms
        @variant("deep_quiet")
        class GliderDeepQuiet(GliderDeep, GliderQuiet, GliderAcomms):
            pass
        """,
    "extra/missions/fleet/pair/pair.py": """
        from missions.platform.topside import Topside
        from missions.platform.auv.glider import Glider
        class Pair(Fleet):
            platforms = [Topside, Glider(modem_id=2, speed=0.4)]
            overrides = {"platform.glider.subsystem.main.component.logging.level": "DEBUG"}
        """,
    "extra/missions/cruise/trial/trial.py": """
        class Trial(Cruise):
            fleet = "pair"
            environment = "flat"
            params = dict(datum=dict(lat=42.5, lon=-70.5))
            overrides = {"platform.glider.speed": 0.5}
        """,
    "extra/missions/cruise/trial/trial_fast.py": """
        from missions.cruise.trial import Trial
        @variant("fast")
        class TrialFast(Trial):
            params = dict(warp=10)
            overrides = {"platform.glider.speed": 2.0}
        """,
}


def write_tree(root: Path, files: dict, toolboxes=("core", "extra")) -> Path:
    (root / ".lamss").mkdir(parents=True, exist_ok=True)
    (root / ".lamss" / "toolboxes").write_text("\n".join(t for t in toolboxes if t != "core") + "\n")
    for rel, text in files.items():
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(HEADER + textwrap.dedent(text) if rel.endswith(".py") else textwrap.dedent(text))
    return root


def registry_for(root: Path, toolboxes=("core", "extra")) -> Registry:
    return Registry([Toolbox(t, root / t / "missions") for t in toolboxes]).load()


@pytest.fixture
def tree(tmp_path):
    """tree(extra_files, remove=[...]) writes BASIC plus extra_files and returns a loaded registry."""

    def make(extra=None, remove=(), toolboxes=("core", "extra")):
        files = {k: v for k, v in BASIC.items() if k not in remove}
        files.update(extra or {})
        write_tree(tmp_path, files, toolboxes)
        return registry_for(tmp_path, toolboxes)

    make.root = tmp_path
    return make
