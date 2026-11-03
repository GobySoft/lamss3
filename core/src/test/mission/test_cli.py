# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""`python3 -m lamss.mission` against a fixture checkout."""

import yaml

from conftest import BASIC, write_tree
from lamss.mission.cli import main


def run(root, monkeypatch, capsys, *argv):
    monkeypatch.setenv("LAMSS_ROOT", str(root))
    status = main(list(argv))
    out, err = capsys.readouterr()
    return status, out, err


def test_list(tmp_path, monkeypatch, capsys):
    write_tree(tmp_path, BASIC)
    status, out, _ = run(tmp_path, monkeypatch, capsys, "list")
    assert status == 0
    assert out.split() == ["trial", "extra", "acomms,", "deep,", "deep_quiet,", "fast,", "quiet"]


def test_list_one_cruise(tmp_path, monkeypatch, capsys):
    write_tree(tmp_path, BASIC)
    status, out, err = run(tmp_path, monkeypatch, capsys, "list", "trial")
    assert status == 0
    assert "glider/main: interprocess, logging, health, sim_only_thing" in out
    assert "viewer" in err


def test_show_and_explain(tmp_path, monkeypatch, capsys):
    write_tree(tmp_path, BASIC)
    status, out, _ = run(tmp_path, monkeypatch, capsys, "show", "trial", "fast", "--set", "warp=3")
    assert status == 0
    assert yaml.safe_load(out)["cruise"]["warp"] == 3
    status, out, _ = run(tmp_path, monkeypatch, capsys, "show", "trial", "--explain", "platform.glider.speed")
    assert status == 0
    lines = out.splitlines()
    assert lines[0] == "platform.glider.speed = 0.5"
    assert lines[-1].split() == ["cruise", "Trial.overrides", "speed", "=", "0.5", "<-", "wins"]


def test_errors_exit_1(tmp_path, monkeypatch, capsys):
    write_tree(tmp_path, BASIC)
    status, _, err = run(tmp_path, monkeypatch, capsys, "show", "nope")
    assert status == 1
    assert "error: no cruise named 'nope'" in err
