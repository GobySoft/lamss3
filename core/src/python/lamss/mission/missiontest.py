# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""`lamss mission test`: runs a cruise's `goby test` files against the mission.

For each test file in the cruise directory's `test/` (or given with --test-file) this writes, under
`missions/build/<instance>/test/`, an include with one endpoint per subsystem that has a Goby
interprocess endpoint (named `<platform>_<subsystem>`) and one fixture that runs the mission's
supervisord in the foreground, ready once every process is RUNNING, plus a wrapper that includes
it and the test file. `goby test run` then runs the wrapper with -D values from the context.
"""

from __future__ import annotations

import random
import shlex
import socket
import subprocess
import sys
from pathlib import Path
from typing import Any, Optional, Sequence

import yaml

from .. import style
from . import render, supervisor
from .errors import MissionError
from .params import flatten
from .resolve import Mission, component_ports

TEST_SUFFIXES = (".yml", ".yaml", ".json", ".pb.cfg")
PORT_RANGE = (20000, 50000)
READY = "all [0-9]+ processes RUNNING"


def test_files(mission: Mission) -> list[Path]:
    """The files in `test/` beside the cruise class (or the classes it derives from)."""
    out: list[Path] = []
    for cls in type(mission.cruise).__mro__:
        d = mission.registry.directory_of(cls)
        if d is None or not (d / "test").is_dir():
            continue
        for f in sorted((d / "test").iterdir()):
            if f.is_file() and f.name.endswith(TEST_SUFFIXES) and f not in out:
                out.append(f)
    return out


def ports_needed(mission: Mission) -> dict[Optional[str], int]:
    """How many ports the mission allocates from port_base (key None) and from each named range,
    supervisord's web page included."""
    counts: dict[Optional[str], int] = {"supervisor": 1}
    for p in mission.platforms:
        for s in p.resolved_subsystems:
            for c in s.components:
                for port_range in component_ports(c).values():
                    counts[port_range] = counts.get(port_range, 0) + 1
    return counts


def _free(port: int) -> bool:
    for kind in (socket.SOCK_STREAM, socket.SOCK_DGRAM):
        with socket.socket(socket.AF_INET, kind) as s:
            try:
                s.bind(("", port))
            except OSError:
                return False
    return True


def free_port_base(count: int, rng: Optional[random.Random] = None) -> int:
    """A base port with `count` consecutive free ports after it, so tests never meet other
    missions (or each other) on the same machine."""
    rng = rng or random.Random()
    for _ in range(200):
        base = rng.randrange(PORT_RANGE[0], PORT_RANGE[1] - count)
        if all(_free(base + i) for i in range(max(count, 1))):
            return base
    raise MissionError(f"found no {count} consecutive free ports in {PORT_RANGE}")


def _scalars(prefix: str, values: dict[str, Any]) -> dict[str, str]:
    out = {}
    for key, value in flatten(values):
        if isinstance(value, bool):
            out[f"{prefix}_{key.replace('.', '_')}"] = str(value).lower()
        elif isinstance(value, (str, int, float)):
            out[f"{prefix}_{key.replace('.', '_')}"] = str(value)
    return out


def defines(mission: Mission, layout: render.Layout, selection: Sequence[render.Selected]) -> dict[str, str]:
    """-D values for the test files: LAMSS_*, CRUISE_<param>, PLATFORM_<p>_<param>,
    PORT_<p>_<s>_<name>, RUN_<p>_<s> and LOG_<p>_<s>."""
    d = mission.to_dict()
    out = {
        "LAMSS_ROOT": str(layout.root),
        "LAMSS_PREFIX": str(layout.prefix),
        "LAMSS_INSTANCE": layout.instance,
        "LAMSS_CRUISE": mission.name,
        "LAMSS_RUN": str(layout.run),
    }
    out.update(_scalars("CRUISE", {k: v for k, v in d["cruise"].items() if k != "name"}))
    for name, platform in d["platform"].items():
        out.update(_scalars(f"PLATFORM_{name}", {k: v for k, v in platform.items()
                                                  if k not in ("name", "subsystem")}))
    for sel in selection:
        p, s = sel.platform.name, sel.subsystem.name
        for port, value in sel.subsystem.allocated_ports.items():
            out[f"PORT_{p}_{s}_{port}"] = str(value)
        out[f"RUN_{p}_{s}"] = str(layout.subsystem_run(p, s))
        out[f"LOG_{p}_{s}"] = str(layout.subsystem_log(p, s))
    return out


def endpoints(renderer: render.Renderer) -> list[dict[str, Any]]:
    out = []
    for sel in renderer.selection:
        paths = renderer.paths(sel.platform, sel.subsystem)
        found = [(c, e) for c in sel.subsystem.components if (e := c.test_endpoint(paths)) is not None]
        if len(found) > 1:
            raise MissionError(f"{sel.subsystem.path}: {found[0][0].name} and {found[1][0].name} "
                               f"both declare a test endpoint")
        if found:
            out.append({"name": f"{sel.platform.name}_{sel.subsystem.name}", **found[0][1]})
    return out


def fixture(mission: Mission, layout: render.Layout, sim_args: Sequence[str], wait: float) -> dict[str, Any]:
    package_dir = str(Path(__file__).resolve().parents[2])
    command = [sys.executable, "-m", "lamss.mission", "sim", *sim_args, "--instance", layout.instance,
               "--execute", "--foreground", "--wait", f"{wait:g}"]
    return {
        "name": f"mission {layout.instance}",
        "command": "exec " + shlex.join(command),
        "cwd": str(layout.root),
        "env": [f"PYTHONPATH={package_dir}", f"LAMSS_ROOT={layout.root}", f"LAMSS_PREFIX={layout.prefix}"],
        "ready": {"output_regex": [READY], "timeout": wait + 15},
        "stop": {"signal": "SIGINT", "grace": 30},
    }


def _yaml(data: Any) -> str:
    return "# Generated by lamss mission test\n" + yaml.safe_dump(data, sort_keys=False, width=1 << 16)


def write(mission: Mission, layout: render.Layout, selection: Sequence[render.Selected],
          files: Sequence[Path], sim_args: Sequence[str], wait: float) -> list[Path]:
    """Writes the include and one wrapper per test file; returns the wrappers."""
    test_dir = layout.build / "test"
    test_dir.mkdir(parents=True, exist_ok=True)
    renderer = render.Renderer(mission, layout, selection)
    include = {"endpoint": endpoints(renderer), "fixture": [fixture(mission, layout, sim_args, wait)]}
    (test_dir / "mission.yml").write_text(_yaml(include))
    wrappers = []
    for f in files:
        wrapper = test_dir / f"{f.name.split('.')[0]}.yml"
        if wrapper in wrappers:
            raise MissionError(f"two test files are named {wrapper.stem}")
        wrapper.write_text(_yaml({"include": ["mission.yml", str(f.resolve())]}))
        wrappers.append(wrapper)
    return wrappers


def run(mission: Mission, layout: render.Layout, selection: Sequence[render.Selected],
        files: Sequence[Path], sim_args: Sequence[str], wait: float, goby: str,
        goby_args: Sequence[str]) -> int:
    if not files:
        raise MissionError(f"cruise {mission.name} has no test files (test/*.yml beside its class); "
                           f"give one with --test-file")
    wrappers = write(mission, layout, selection, files, sim_args, wait)
    values = defines(mission, layout, selection)
    libs = render.proto_libraries(layout.root, layout.prefix, [t.name for t in mission.registry.toolboxes])
    worst = 0
    for wrapper, original in zip(wrappers, files):
        command = [goby, "test", "run", str(wrapper)]
        for key, value in values.items():
            command += ["-D", f"{key}={value}"]
        for lib in libs:
            command += ["-l", lib]
        if not any(a == "--log_dir" or a.startswith("--log_dir=") for a in goby_args):
            command += ["--log_dir", str(layout.run / "goby_test" / wrapper.stem)]
        command += list(goby_args)
        print("lamss mission test: " + style.heading(str(original), sys.stderr), file=sys.stderr,
              flush=True)
        try:
            status = subprocess.call(command)
        except FileNotFoundError:
            raise MissionError(f"cannot run {goby}; is goby3-apps installed?") from None
        finally:
            supervisor.stop(layout)
        worst = max(worst, status)
    return worst
