# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""`lamss mission test` on the fixtures/goby checkout: two platforms, each with its own gobyd,
checked by a goby test file that uses the generated endpoints, fixture and -D values."""

import json
import shutil
import socket
import subprocess
from pathlib import Path

import pytest
import yaml

from lamss.mission import missiontest
from lamss.mission.cli import main

FIXTURE = Path(__file__).parent / "fixtures" / "goby"


def goby_test_available() -> bool:
    if not (shutil.which("goby") and shutil.which("gobyd") and shutil.which("supervisord")):
        return False
    return subprocess.run(["goby", "test", "--help"], capture_output=True).returncode == 0


needs_goby = pytest.mark.skipif(not goby_test_available(), reason="needs goby test, gobyd, supervisord")


@pytest.fixture
def checkout(tmp_path, monkeypatch):
    root = tmp_path / "c"
    shutil.copytree(FIXTURE, root)
    monkeypatch.setenv("LAMSS_ROOT", str(root))
    return root


def test_free_port_base_skips_ports_in_use():
    with socket.socket() as s:
        s.bind(("", 0))
        busy = s.getsockname()[1]
        base = missiontest.free_port_base(3)
        assert busy not in range(base, base + 3)
        assert missiontest.PORT_RANGE[0] <= base < missiontest.PORT_RANGE[1]


@needs_goby
def test_pass_with_generated_endpoints_and_defines(checkout, tmp_path):
    report = tmp_path / "report.json"
    assert main(["test", "pair", "--", "-o", "JSON", "--output_file", str(report)]) == 0
    assert json.loads(report.read_text())["result"] == "PASS"
    include = yaml.safe_load((checkout / "core/missions/build/pair_test/test/mission.yml").read_text())
    assert [e["name"] for e in include["endpoint"]] == ["alpha_main", "beta_main"]
    assert include["fixture"][0]["ready"]["output_regex"] == [missiontest.READY]
    assert not _running(checkout)


@needs_goby
def test_failing_expectation_exits_1(checkout, tmp_path):
    failing = tmp_path / "failing.yml"
    failing.write_text(yaml.safe_dump({"test": {"name": "beta hears alpha", "run": [{
        "name": "crossed",
        "publish": [{"endpoint": "alpha_main", "group": "g", "type": "JSON/X", "value": "{}", "delay": 0.5}],
        "expect": [{"endpoint": "beta_main", "group": "g", "timeout": 1.5}]}]}}))
    assert main(["test", "pair", "--test-file", str(failing), "--instance", "failing"]) == 1
    assert not _running(checkout)


def test_no_test_files(checkout, capsys):
    shutil.rmtree(checkout / "core/missions/cruise/pair/test")
    assert main(["test", "pair"]) == 1
    assert "has no test files" in capsys.readouterr().err


def test_defines(checkout):
    from lamss.mission import render, resolve, workspace
    from lamss.mission.registry import Registry
    registry = Registry(workspace.toolboxes(checkout)).load()
    mission = resolve(registry, "pair", sets=["port_base=31000"])
    layout = render.layout_for(mission, checkout, checkout / "build" / "usr", "x")
    d = missiontest.defines(mission, layout, render.select(mission))
    assert d["LAMSS_CRUISE"] == "pair" and d["CRUISE_port_base"] == "31000"
    assert d["CRUISE_datum_lat"] == "0.0" and d["PLATFORM_alpha_type"] == "platform"
    assert d["RUN_beta_main"].endswith("run/x/beta/main")


def _running(root: Path) -> bool:
    for proc in Path("/proc").iterdir():
        try:
            if proc.name.isdigit() and f"LAMSS_ROOT={root}".encode() in (proc / "environ").read_bytes().split(b"\0"):
                return True
        except OSError:
            pass
    return False
