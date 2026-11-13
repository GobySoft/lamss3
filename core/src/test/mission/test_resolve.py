# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Resolving a cruise: tree shape, parameter precedence, variants, --set and validation."""

import pytest

from lamss.mission import MissionError, resolve


def test_tree_shape(tree):
    m = resolve(tree(), "trial")
    assert m.name == "trial"
    assert [p.name for p in m.platforms] == ["topside", "glider"]
    glider = m.platform("glider")
    assert [s.name for s in glider.resolved_subsystems] == ["main"]
    assert [c.name for c in glider.resolved_subsystems[0].components] == [
        "interprocess", "logging", "health", "sim_only_thing"]
    assert m.environment.name == "flat"


def test_precedence_component_platform_fleet_cruise_variant_set(tree):
    registry = tree()
    m = resolve(registry, "trial")
    d = m.to_dict()
    glider = d["platform"]["glider"]
    assert glider["max_depth"] == 200            # Glider over Auv (MRO)
    assert glider["speed"] == 0.5                # cruise override over fleet spec over platform
    assert glider["modem_id"] == 2               # fleet spec
    assert glider["subsystem"]["main"]["component"]["logging"]["level"] == "DEBUG"  # fleet overrides
    assert d["platform"]["topside"]["subsystem"]["main"]["component"]["logging"]["level"] == "WARN"

    m = resolve(registry, "trial", "fast")
    assert m.to_dict()["platform"]["glider"]["speed"] == 2.0  # variant over cruise
    assert m.to_dict()["cruise"]["warp"] == 10

    m = resolve(registry, "trial", "fast", sets=["platform.glider.speed=3", "warp=20"])
    assert m.to_dict()["platform"]["glider"]["speed"] == 3   # --set over everything
    assert m.to_dict()["cruise"]["warp"] == 20                # bare keys are cruise parameters


def test_platform_variant_params_beat_cruise_overrides(tree):
    m = resolve(tree({"extra/missions/cruise/trial/trial.py": """
        class Trial(Cruise):
            fleet = "pair"
            overrides = {"platform.glider.max_depth": 300}
        """}), "trial", "deep")
    assert m.platform("glider").params["max_depth"] == 1000


def test_explain_names_every_layer(tree):
    m = resolve(tree(), "trial", "fast", sets=["platform.glider.speed=3"])
    value, layers = m.explain("platform.glider.speed")
    assert value == 3
    assert [(e.kind, e.source) for e in layers] == [
        ("platform", "Auv.params"), ("platform", "Glider.params"), ("fleet", "Pair.platforms"),
        ("cruise", "Trial.overrides"), ("variant", "TrialFast.overrides"),
        ("set", "--set platform.glider.speed")]
    assert [e.wins for e in layers] == [False] * 5 + [True]


def test_explain_unknown_key(tree):
    m = resolve(tree(), "trial")
    with pytest.raises(MissionError, match="has no value 'max_dept'"):
        m.explain("platform.glider.max_dept")
    with pytest.raises(MissionError, match="names nothing"):
        m.explain("platform.nope.max_depth")


def test_set_of_unknown_parameter_is_an_error(tree):
    with pytest.raises(MissionError, match="sets 'max_dept'.*known:.*max_depth"):
        resolve(tree(), "trial", sets=["platform.glider.max_dept=3"])


def test_set_values_are_yaml(tree):
    m = resolve(tree(), "trial", sets=["cruise.datum={lat: 1, lon: 2}", "platform.topside.type=ship"])
    assert m.cruise.params["datum"] == {"lat": 1, "lon": 2}
    assert m.platform("topside").params["type"] == "ship"


def test_variant_removes_a_component(tree):
    m = resolve(tree(), "trial", "quiet")
    assert "health" not in [c.name for c in m.platform("glider").resolved_subsystems[0].components]


def test_combined_variant(tree):
    m = resolve(tree(), "trial", "deep_quiet")
    glider = m.platform("glider")
    assert glider.params["max_depth"] == 1000
    comps = {c.name: c for c in glider.resolved_subsystems[0].components}
    assert list(comps) == ["interprocess", "logging", "sim_only_thing", "acomms"]
    assert comps["acomms"].params == {"driver": "udp", "port": 12000}
    assert type(glider).__name__ == "GliderDeepQuiet"


def test_component_named_after_its_role(tree):
    m = resolve(tree(), "trial", "acomms")
    assert "acomms" in m.to_dict()["platform"]["glider"]["subsystem"]["main"]["component"]


def test_unknown_or_inapplicable_variant(tree):
    with pytest.raises(MissionError, match="no variant named 'nope'"):
        resolve(tree(), "trial", "nope")
    registry = tree({"extra/missions/fleet/pair/pair.py": """
        from missions.platform.topside import Topside
        class Pair(Fleet):
            platforms = [Topside]
        """})
    with pytest.raises(MissionError, match="variant 'deep' does not apply to cruise trial"):
        resolve(registry, "trial", "deep")


def test_run_mode_drops_sim_only_and_environment(tree):
    m = resolve(tree(), "trial", mode="run")
    assert m.environment is None
    assert "sim_only_thing" not in [c.name for c in m.platform("glider").resolved_subsystems[0].components]


def test_optional_component_is_left_out_with_a_note(tree):
    m = resolve(tree(), "trial")
    assert any("optional component 'viewer' left out" in n for n in m.notes)


def test_optional_component_is_used_when_present(tree):
    m = resolve(tree({"extra/missions/component/viewer/viewer.py": "class Viewer(Component): pass\n"}), "trial")
    assert "viewer" in [c.name for c in m.platform("topside").resolved_subsystems[0].components]
    assert not m.notes


def test_missing_requirement(tree):
    registry = tree({"extra/missions/platform/auv/glider/glider.py": """
        from missions.platform.auv import Auv
        from missions.component.logging import Logging
        class Glider(Auv):
            components = [Logging]
        """}, remove=[k for k in __import__("conftest").BASIC if "glider_" in k])
    with pytest.raises(MissionError, match="logging requires Interprocess"):
        resolve(registry, "trial")


def test_duplicate_modem_id(tree):
    with pytest.raises(MissionError, match="both have modem_id 2"):
        resolve(tree(), "trial", sets=["platform.topside.modem_id=2"])


def test_two_platforms_of_one_class_need_names(tree):
    files = {"extra/missions/fleet/pair/pair.py": """
        from missions.platform.auv.glider import Glider
        class Pair(Fleet):
            platforms = [Glider, Glider]
        """,
             "extra/missions/cruise/trial/trial.py": "class Trial(Cruise):\n    fleet = 'pair'\n"}
    with pytest.raises(MissionError, match="two platforms named 'glider'"):
        resolve(tree(files), "trial")


def test_named_platform_specs(tree):
    files = {"extra/missions/fleet/pair/pair.py": """
        from missions.platform.auv.glider import Glider
        class Pair(Fleet):
            platforms = [Glider("g1", modem_id=1), Glider("g2", modem_id=2)]
        """,
             "extra/missions/cruise/trial/trial.py": "class Trial(Cruise):\n    fleet = 'pair'\n"}
    m = resolve(tree(files), "trial", sets=["platform.g2.max_depth=5"])
    assert [(p.name, p.params["max_depth"]) for p in m.platforms] == [("g1", 200), ("g2", 5)]


def test_subsystems_by_mode(tree):
    files = {"extra/missions/platform/auv/glider/glider.py": """
        from missions.platform.auv import Auv
        from missions.component.interprocess import Interprocess
        class Glider(Auv):
            subsystems = Auv.subsystems + [Subsystem("frontseat_sim", [Interprocess], mode="sim", host="sim")]
        """}
    registry = tree(files, remove=[k for k in __import__("conftest").BASIC if "glider_" in k])
    sim = resolve(registry, "trial")
    assert [s.name for s in sim.platform("glider").resolved_subsystems] == ["main", "frontseat_sim"]
    assert sim.to_dict()["platform"]["glider"]["subsystem"]["frontseat_sim"]["host"] == "sim"
    run = resolve(registry, "trial", mode="run")
    assert [s.name for s in run.platform("glider").resolved_subsystems] == ["main"]


def test_derived_values_reach_the_dict(tree):
    m = resolve(tree({"extra/missions/component/depth/depth.py": """
        class Depth(Component):
            params = dict(feet=10)
            def derived(self):
                return {"meters": round(self.params["feet"] * 0.3048, 3),
                        "platform_max": self.platform.params["max_depth"]}
        """,
        "extra/missions/platform/auv/glider/glider.py": """
        from missions.platform.auv import Auv
        class Glider(Auv):
            params = dict(max_depth=200)
            components = Auv.components + ["depth"]
        """}, remove=[k for k in __import__("conftest").BASIC if "glider_" in k]), "trial")
    depth = m.to_dict()["platform"]["glider"]["subsystem"]["main"]["component"]["depth"]
    assert depth == {"name": "depth", "feet": 10, "meters": 3.048, "platform_max": 200}


def test_to_dict_is_plain_data(tree):
    import yaml
    d = resolve(tree(), "trial").to_dict()
    assert yaml.safe_load(yaml.safe_dump(d)) == d


WEB = {
    "core/missions/component/web/web.py": """
        class Web(Component):
            ports = {"web": "http", "db": None}
        """,
    "core/missions/platform/auv/auv.py": """
        from missions.component.interprocess import Interprocess
        from missions.component.logging import Logging
        from missions.component.web import Web
        class Auv(Platform):
            params = dict(type="auv", max_depth=100, speed=1.5)
            components = [Interprocess, Logging, Web]
        """,
}


def test_ports_from_named_ranges(tree):
    """A port named with a range comes from cruise.port_ranges, the others from port_base, so web
    pages stay clear of the MOOS ports."""
    ports = resolve(tree(WEB), "trial").platform("glider").resolved_subsystems[0].allocated_ports
    assert ports == {"db": 9000, "web": 50000}
    moved = resolve(tree(WEB), "trial", sets=["port_ranges.http=60000"])
    assert moved.platform("glider").resolved_subsystems[0].allocated_ports["web"] == 60000


def test_port_range_must_exist(tree):
    files = dict(WEB)
    files["core/missions/component/web/web.py"] = """
        class Web(Component):
            ports = {"web": "nope"}
        """
    with pytest.raises(MissionError, match="range 'nope', which cruise.port_ranges does not define"):
        resolve(tree(files), "trial")


def test_every_key_of_the_mission_can_be_explained(tree):
    """`keys()` lists each concept and each value `show` prints, and `explain` accounts for all of
    them, so `show --explain-all` covers the whole mission."""
    m = resolve(tree(), "trial", "deep", sets=["platform.glider.speed=3"])
    keys = m.keys()
    assert keys[:3] == ["mode", "variant", "cruise"]
    assert "platform.glider.subsystem.main.component.logging.level" in keys
    for key in keys:
        m.explain(key)


def test_explain_concepts_names_and_command(tree):
    m = resolve(tree(), "trial", "deep")
    value, layers = m.explain("platform.glider")
    assert value == "GliderDeep"
    assert [(e.kind, e.source) for e in layers][:2] == [
        ("listed", "Pair.platforms"), ("variant", "GliderDeep replaces Glider (variant deep)")]
    assert layers[-1].kind == "class" and "extra:platform/auv/glider" in layers[-1].source
    value, layers = m.explain("platform.glider.subsystem.main.component.health")
    assert value == "Health" and layers[0].source == "Glider.components"
    assert m.explain("variant") == ("deep", [m.explain("variant")[1][0]])
    assert m.explain("variant")[1][0].kind == "command"
    assert m.explain("fleet.platforms")[0] == ["topside", "glider"]
    assert m.explain("platform.glider.name")[1][0].kind == "name"


def test_explain_derived_value(tree):
    registry = tree({"core/missions/component/logging/logging.py": """
        from missions.component.interprocess import Interprocess
        class Logging(Component):
            requires = [Interprocess]
            params = dict(level="INFO", dir="log")
            def derived(self):
                return {"path": self.params["dir"] + "/" + self.name}
        """})
    value, layers = resolve(registry, "trial").explain(
        "platform.glider.subsystem.main.component.logging.path")
    assert value == "log/logging"
    assert [(e.kind, e.source) for e in layers] == [("derived", "Logging.derived()")]
