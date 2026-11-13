# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Starting and stopping the fixtures/render mission under a real supervisord (its processes are
`sleep`), in the background and in the foreground, and checking nothing is left running."""

import os
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path

import pytest

from lamss.mission import supervisor
from lamss.mission.cli import main

pytestmark = pytest.mark.skipif(shutil.which("supervisord") is None, reason="needs supervisor")

FIXTURE = Path(__file__).parent / "fixtures" / "render"


@pytest.fixture
def checkout(tmp_path, monkeypatch):
    root = tmp_path / "c"
    shutil.copytree(FIXTURE, root)
    monkeypatch.setenv("LAMSS_ROOT", str(root))
    yield root
    main(["stop", "harbor"])


def pids(root: Path) -> list[int]:
    out = []
    for proc in Path("/proc").iterdir():
        if proc.name.isdigit():
            try:
                environ = (proc / "environ").read_bytes()
            except OSError:
                continue
            if f"LAMSS_ROOT={root}".encode() in environ.split(b"\0"):
                out.append(int(proc.name))
    return out


def test_sim_status_stop(checkout, capsys):
    assert main(["sim", "harbor", "--wait", "20"]) == 0
    assert "harbor: 7 processes RUNNING" in capsys.readouterr().out
    assert len(pids(checkout)) == 8  # supervisord and its seven programs
    assert main(["status"]) == 0
    status = capsys.readouterr().out
    assert status.count("RUNNING") == 7 and "buoy_sensors:daemon" in status
    assert main(["sim", "harbor"]) == 1
    assert "already running" in capsys.readouterr().err
    log = checkout / "extra" / "missions" / "run" / "harbor" / "log"
    assert log.is_symlink() and (log / "supervisord.log").exists() and (log / "buoy" / "sensors").is_dir()
    assert main(["stop"]) == 0
    assert pids(checkout) == []
    assert main(["status"]) == 3


def test_failing_process_is_reported(checkout, capsys):
    assert main(["sim", "harbor", "--set", "platform.shore.subsystem.main.component.daemon.seconds=x",
                 "--wait", "20"]) == 1
    assert "shore_main:daemon" in capsys.readouterr().err
    assert pids(checkout) == []


def test_foreground_until_interrupted(checkout):
    env = dict(os.environ, PYTHONPATH=os.pathsep.join(sys.path))
    process = subprocess.Popen([sys.executable, "-m", "lamss.mission", "sim", "harbor", "--foreground"],
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, env=env,
                               start_new_session=True)
    deadline = time.monotonic() + 30
    lines = []
    for line in process.stdout:
        lines.append(line)
        if "all 7 processes RUNNING" in line or time.monotonic() > deadline:
            break
    assert any("lamss mission: harbor: all 7 processes RUNNING" in line for line in lines), lines
    os.killpg(process.pid, signal.SIGINT)
    assert process.wait(timeout=30) == 0
    assert pids(checkout) == []


def test_launch_without_fixed_http_port_gets_a_local_web_page(tmp_path):
    from lamss.mission.render import Layout

    layout = Layout(root=tmp_path, prefix=tmp_path, missions=tmp_path, instance="harbor")
    layout.run.mkdir(parents=True)
    url = supervisor.prepare_http(layout, None, 50000, avoid=range(50000, 50010))
    port = int(url.rsplit(":", 1)[1].rstrip("/"))
    assert url.startswith("http://127.0.0.1:") and port >= 50010
    assert f"port=127.0.0.1:{port}" in (layout.run / "http.conf").read_text()
    assert supervisor.http_url(layout) == url
    supervisor.prepare_http(layout, 9123, 50000)
    assert not (layout.run / "http.conf").exists()
    assert supervisor.http_url(layout).endswith(":9123/")
