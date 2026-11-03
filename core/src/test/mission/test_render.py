# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Rendering the fixtures/render checkout: golden files, reproducibility, template errors.

Golden files live in fixtures/render_expected with the checkout path replaced by @ROOT@. After an
intended change, regenerate them with LAMSS_UPDATE_GOLDEN=1 and review the diff."""

import os
import shutil
from pathlib import Path

import pytest

from lamss.mission import MissionError, resolve
from lamss.mission import render, workspace
from lamss.mission.cli import main
from lamss.mission.registry import Registry

HERE = Path(__file__).parent
FIXTURE = HERE / "fixtures" / "render"
EXPECTED = HERE / "fixtures" / "render_expected"


@pytest.fixture
def checkout(tmp_path, monkeypatch):
    root = tmp_path / "c"
    shutil.copytree(FIXTURE, root)
    monkeypatch.setenv("LAMSS_ROOT", str(root))
    monkeypatch.delenv("LAMSS_PREFIX", raising=False)
    return root


def tree(directory: Path, root: Path) -> dict:
    return {str(p.relative_to(directory)): p.read_text().replace(str(root), "@ROOT@")
            for p in sorted(directory.rglob("*")) if p.is_file()}


def generate(root, *args):
    assert main(["generate", *args]) == 0
    return root / "extra" / "missions" / "build"


def test_golden(checkout):
    out = tree(generate(checkout, "harbor"), checkout)
    if os.environ.get("LAMSS_UPDATE_GOLDEN"):
        shutil.rmtree(EXPECTED, ignore_errors=True)
        for rel, text in out.items():
            (EXPECTED / rel).parent.mkdir(parents=True, exist_ok=True)
            (EXPECTED / rel).write_text(text)
    expected = tree(EXPECTED, checkout)
    assert sorted(out) == sorted(expected)
    for rel in expected:
        assert out[rel] == expected[rel], rel


def test_generating_twice_is_byte_identical(checkout):
    build = generate(checkout, "harbor", "quick")
    first = {p: p.read_bytes() for p in build.rglob("*") if p.is_file()}
    generate(checkout, "harbor", "quick")
    second = {p: p.read_bytes() for p in build.rglob("*") if p.is_file()}
    assert first == second


def test_variant_and_set_choose_the_instance(checkout):
    build = generate(checkout, "harbor", "quick", "--set", "warp=7")
    assert (build / "harbor_quick" / "shore" / "main" / "env.sh").read_text().count("LAMSS_WARP=7") == 1
    generate(checkout, "harbor", "--instance", "other")
    assert (build / "other" / "sim" / "supervisord.conf").is_file()


def test_selection(checkout):
    build = generate(checkout, "harbor", "--platform", "buoy", "--subsystem", "sensors")
    assert sorted(p.name for p in (build / "harbor").iterdir()) == ["buoy", "mission.yaml", "sim"]
    assert "files=../buoy/sensors/supervisord.conf" in (build / "harbor" / "sim" / "supervisord.conf").read_text()


def test_run_mode_needs_a_platform(checkout, capsys):
    assert main(["generate", "harbor", "--mode", "run"]) == 1
    assert "run needs --platform" in capsys.readouterr().err
    build = generate(checkout, "harbor", "--mode", "run", "--platform", "buoy")
    assert sorted(p.name for p in (build / "harbor" / "buoy").iterdir()) == ["main"]
    assert (build / "harbor" / "run" / "supervisord.conf").is_file()


def test_generate_explain(checkout, capsys):
    assert main(["generate", "harbor", "--explain", "port_base"]) == 0
    assert capsys.readouterr().out.splitlines()[0] == "port_base = 24000"


def test_undefined_template_variable_is_an_error(checkout, capsys):
    template = checkout / "core" / "missions" / "templates" / "daemon.cfg.j2"
    template.write_text(template.read_text() + "{{ component.daemon.missing }}\n")
    assert main(["generate", "harbor"]) == 1
    err = capsys.readouterr().err
    assert "daemon.cfg.j2" in err and "missing" in err and "platform.shore.subsystem.main.component.daemon" in err


def test_cruise_directory_templates_override(checkout):
    (checkout / "extra" / "missions" / "cruise" / "harbor" / "daemon.cfg.j2").write_text("override\n")
    build = generate(checkout, "harbor")
    assert (build / "harbor" / "shore" / "main" / "daemon.cfg").read_text() == "override\n"


def test_missing_template(checkout, capsys):
    (checkout / "core" / "missions" / "templates" / "daemon.cfg.j2").unlink()
    assert main(["generate", "harbor"]) == 1
    assert "template daemon.cfg.j2 not found" in capsys.readouterr().err


def test_explicit_ports_are_kept_and_skipped(checkout):
    build = generate(checkout, "harbor", "--set", "platform.shore.subsystem.main.ports.moosdb=24002")
    ctx = (build / "harbor" / "buoy" / "main" / "last.moos").read_text()
    assert "ServerPort = 24003" in ctx
    assert "ServerPort = 24002" in (build / "harbor" / "shore" / "main" / "last.moos").read_text()


def test_filters():
    assert render.pb_string('a "b"\\\n') == '"a \\"b\\"\\\\\\n"'
    assert render.latlon({"lat": 1.5, "lon": -2}) == "lat: 1.5 lon: -2"
    assert render.utm({"lat": 0, "lon": 3}) == {"zone": 31, "hemisphere": "N", "easting": 500000.0,
                                                 "northing": 0.0}
    # on the central meridian, northing is k0 times the meridian arc (4984944.378 m to 45 deg)
    north = render.utm({"lat": 45, "lon": -69})
    assert (north["zone"], north["easting"]) == (19, 500000.0)
    assert abs(north["northing"] - 0.9996 * 4984944.378) < 0.01
    south = render.utm({"lat": -45, "lon": -69})
    assert south["hemisphere"] == "S" and abs(south["northing"] + north["northing"] - 1e7) < 0.01
    assert render.utm({"lat": 42.358, "lon": -71.087})["easting"] < 500000


def test_long_socket_paths_fall_back(tmp_path):
    long_dir = tmp_path / ("x" * 120)
    path = render.socket_path(long_dir, "supervisor.sock")
    assert len(str(path)) <= render.MAX_SOCKET_PATH
    assert path == render.socket_path(long_dir, "supervisor.sock")
    assert render.socket_path(tmp_path, "s.sock") == tmp_path / "s.sock"
