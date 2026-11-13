# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Starts, watches and stops one instance's supervisord."""

from __future__ import annotations

import os
import signal
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional, Sequence

from .. import style
from .errors import MissionError
from .render import HTTP_CONF, HTTP_URL, Layout

GOOD = "RUNNING"
STARTING = ("STARTING", "BACKOFF")


@dataclass(frozen=True)
class ProgramState:
    name: str
    state: str
    description: str


def supervisorctl(layout: Layout, *args: str, capture: bool = True) -> subprocess.CompletedProcess:
    return subprocess.run(["supervisorctl", "-s", f"unix://{layout.socket}", *args],
                          capture_output=capture, text=True)


def states(layout: Layout) -> Optional[list[ProgramState]]:
    """Every program's state, or None when supervisord is not answering."""
    if not layout.socket.exists():
        return None
    result = supervisorctl(layout, "status")
    if "refused connection" in result.stdout + result.stderr or "no such file" in result.stdout + result.stderr:
        return None
    out = []
    for line in result.stdout.splitlines():
        parts = line.split(None, 2)
        if len(parts) >= 2:
            out.append(ProgramState(parts[0], parts[1], parts[2] if len(parts) > 2 else ""))
    return out


def supervisord_pid(layout: Layout) -> Optional[int]:
    try:
        pid = int(layout.pidfile.read_text().strip())
        os.kill(pid, 0)
        return pid
    except (OSError, ValueError):
        return None


def running(layout: Layout) -> bool:
    return supervisord_pid(layout) is not None and states(layout) is not None


def prepare(layout: Layout, subsystems: Sequence[tuple[str, str]]) -> Path:
    """Creates the run directories and a new timestamped log directory, linked as run/log."""
    layout.run.mkdir(parents=True, exist_ok=True)
    layout.socket.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H-%M-%SZ")
    log_dir = layout.logs / stamp
    log_dir.mkdir(parents=True, exist_ok=True)
    link = layout.run / "log"
    if link.is_symlink() or link.exists():
        link.unlink()
    link.symlink_to(os.path.relpath(log_dir, layout.run))
    for platform, subsystem in subsystems:
        layout.subsystem_run(platform, subsystem).mkdir(parents=True, exist_ok=True)
        layout.subsystem_log(platform, subsystem).mkdir(parents=True, exist_ok=True)
    return log_dir


def free_port(first: int, avoid: Sequence[int] = (), count: int = 1000) -> int:
    """The first port from `first` that is free on localhost and not in `avoid`."""
    last = first + count - 1
    for port in range(first, last + 1):
        if port in avoid:
            continue
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
            try:
                probe.bind(("127.0.0.1", port))
                return port
            except OSError:
                continue
    raise MissionError(f"no free port for supervisord's web page in {first}-{last}")


def prepare_http(layout: Layout, http_port: Optional[int], first: int,
                 avoid: Sequence[int] = ()) -> str:
    """Gives a launch without a fixed supervisor.http_port a web page on a free localhost port from
    `first` (not one the mission allocated), and records the page's address for `status`."""
    conf = layout.run / HTTP_CONF
    if http_port:
        conf.unlink(missing_ok=True)
        url = f"http://{socket.gethostname()}:{int(http_port)}/"
    else:
        port = free_port(first, avoid)
        conf.write_text(f"[inet_http_server]\nport=127.0.0.1:{port}\n")
        url = f"http://127.0.0.1:{port}/"
    (layout.run / HTTP_URL).write_text(url + "\n")
    return url


def http_url(layout: Layout) -> Optional[str]:
    """The address of supervisord's web page for this instance, as its launch recorded it."""
    path = layout.run / HTTP_URL
    return path.read_text().strip() if path.is_file() else None


def wait_running(layout: Layout, timeout: float, process: Optional[subprocess.Popen] = None) -> list[ProgramState]:
    """Waits until every program is RUNNING; raises MissionError when one fails or time runs out."""
    deadline = time.monotonic() + timeout
    last: Optional[list[ProgramState]] = None
    while time.monotonic() < deadline:
        if process is not None and process.poll() is not None:
            raise MissionError(f"supervisord exited with status {process.returncode}; see "
                               f"{layout.run / 'log' / 'supervisord.log'}")
        last = states(layout)
        if last:
            failed = [s for s in last if s.state not in (GOOD,) + STARTING]
            if failed:
                raise MissionError("not running: " + ", ".join(f"{s.name} {s.state}" for s in failed)
                                   + f" (logs in {layout.run / 'log'})")
            if all(s.state == GOOD for s in last):
                return last
        time.sleep(0.25)
    pending = ", ".join(f"{s.name} {s.state}" for s in last or [] if s.state != GOOD) or "supervisord"
    raise MissionError(f"not RUNNING after {timeout:g} s: {pending}")


def start(layout: Layout, conf: Path, timeout: float) -> list[ProgramState]:
    if running(layout):
        raise MissionError(f"{layout.instance} is already running; `lamss mission stop {layout.instance}` first")
    if layout.socket.exists():
        layout.socket.unlink()
    result = subprocess.run(["supervisord", "-c", str(conf)], capture_output=True, text=True)
    if result.returncode != 0:
        raise MissionError(f"supervisord failed: {(result.stderr or result.stdout).strip()}")
    try:
        return wait_running(layout, timeout)
    except MissionError:
        stop(layout)
        raise


def run_foreground(layout: Layout, conf: Path, timeout: float) -> int:
    """Runs supervisord in the foreground, prints a line once every program is RUNNING, and stops
    it on SIGINT or SIGTERM; the readiness line is what test fixtures wait for."""
    if running(layout):
        raise MissionError(f"{layout.instance} is already running; `lamss mission stop {layout.instance}` first")
    if layout.socket.exists():
        layout.socket.unlink()
    process = subprocess.Popen(["supervisord", "-n", "-c", str(conf)])
    stopping = False

    def forward(signum, frame):
        nonlocal stopping
        stopping = True
        if process.poll() is None:
            process.send_signal(signal.SIGTERM)

    signal.signal(signal.SIGINT, forward)
    signal.signal(signal.SIGTERM, forward)
    try:
        programs = wait_running(layout, timeout, process)
        print("lamss mission: " + style.good(f"{layout.instance}: all {len(programs)} processes RUNNING"),
              flush=True)
    except MissionError as e:
        if not stopping:
            style.print_error("lamss mission", e)
            forward(None, None)
        process.wait()
        return 1
    while True:
        try:
            return process.wait()
        except KeyboardInterrupt:
            forward(None, None)


def stop(layout: Layout, timeout: float = 60) -> bool:
    """Shuts supervisord down (which stops every program) and waits for it to exit."""
    pid = supervisord_pid(layout)
    if pid is None:
        return False
    supervisorctl(layout, "shutdown")
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            os.kill(pid, 0)
        except OSError:
            return True
        time.sleep(0.2)
    os.kill(pid, signal.SIGKILL)
    return True


def find_running(missions_dirs: Sequence[Path]) -> list[Layout]:
    """Layouts of every instance whose supervisord is alive, in the given toolboxes."""
    out = []
    for missions in missions_dirs:
        run = missions / "run"
        if not run.is_dir():
            continue
        for d in sorted(run.iterdir()):
            layout = Layout(missions.parent.parent, Path(), missions, d.name)
            if supervisord_pid(layout) is not None:
                out.append(layout)
    return out
