# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""The real core (and gui) missions trees: the example cruise resolves and renders, with and
without the viewer, and the MOOS half of the interprocess component can be turned off."""

from pathlib import Path

import pytest

from lamss.mission import MissionError, Registry, Toolbox, render, resolve

ROOT = Path(__file__).resolve().parents[4]


def registry(*toolboxes):
    return Registry([Toolbox(t, ROOT / t / "missions") for t in ("core",) + toolboxes]).load()


def processes(mission, platform="topside", subsystem="main"):
    s = next(s for s in mission.platform(platform).resolved_subsystems if s.name == subsystem)
    return [p.name for c in s.components for p in c.active_processes()]


def write(mission, tmp_path):
    layout = render.Layout(ROOT, tmp_path / "usr", tmp_path / "missions", "example")
    render.generate(mission, layout)
    return {p.relative_to(layout.build): p.read_text()
            for p in sorted(layout.build.rglob("*")) if p.is_file()}


def test_example_without_gui():
    m = resolve(registry(), "example")
    assert processes(m) == ["gobyd", "MOOSDB", "goby_moos_gateway", "goby_coroner", "goby_logger",
                            "pViewMessage"]
    assert any("'viewer' left out" in n for n in m.notes)
    health = m.to_dict()["platform"]["topside"]["subsystem"]["main"]["component"]["health"]
    assert health["expected"] == ["gobyd", "goby_moos_gateway", "goby_coroner", "goby_logger"]


def test_example_with_gui(monkeypatch):
    monkeypatch.delenv("DISPLAY", raising=False)
    monkeypatch.delenv("WAYLAND_DISPLAY", raising=False)
    m = resolve(registry("gui"), "example")
    assert processes(m)[-1] == "goby_liaison"
    assert not m.notes
    # liaison's web page comes from the http range, clear of the MOOS ports from port_base
    ports = m.platform("topside").resolved_subsystems[0].allocated_ports
    assert ports["liaison"] == 50000 and ports["moosdb"] == 9000


def test_moos_off_needs_no_moos_apps():
    # resolving alone reports it, so `lamss mission list` and `show` do too
    with pytest.raises(MissionError, match="view_message is a MOOS app"):
        resolve(registry(), "example",
                sets=["platform.topside.subsystem.main.component.interprocess.moos=false"])


def test_render_is_reproducible_and_complete(tmp_path):
    first = write(resolve(registry("gui"), "example"), tmp_path / "a")
    second = write(resolve(registry("gui"), "example"), tmp_path / "a")
    assert first == second
    main = Path("topside/main")
    assert {p.name for p in first if p.parent == main} == {
        "context.yaml", "env.sh", "supervisord.conf", "gobyd.pb.cfg", "goby_moos_gateway.pb.cfg",
        "goby_coroner.pb.cfg", "goby_logger.pb.cfg", "last.moos", "liaison.pb.cfg"}
    assert "ProcessConfig = pViewMessage" in first[main / "last.moos"]
    assert 'expected_name: "goby_moos_gateway"' in first[main / "goby_coroner.pb.cfg"]


def test_warp_switches_goby_apps_to_sim_time(tmp_path):
    out = write(resolve(registry(), "example", sets=["warp=10"]), tmp_path)
    assert "warp_factor: 10" in out[Path("topside/main/gobyd.pb.cfg")]
    assert "MOOSTimeWarp = 10" in out[Path("topside/main/last.moos")]


def test_moos_off_allocates_no_moosdb_port():
    registry()
    from missions.component.interprocess import Interprocess

    interprocess = Interprocess()
    interprocess.params = {"moos": True, "verbosity": "WARN"}
    assert interprocess.active_ports() == ["moosdb"]
    interprocess.params = {"moos": False, "verbosity": "WARN"}
    assert interprocess.active_ports() == []


def test_every_value_of_the_example_explains():
    for toolboxes in ((), ("gui",)):
        mission = resolve(registry(*toolboxes), "example")
        explained = {key: mission.explain(key)[1][-1] for key in mission.keys()}
        assert [k for k, e in explained.items() if e.location is None] == ["mode", "variant"]
        assert explained["platform.topside.subsystem.main.ports.moosdb"].source == "cruise.port_base"
        assert explained["platform.topside.subsystem.main.component.interprocess.community"].kind == "derived"
