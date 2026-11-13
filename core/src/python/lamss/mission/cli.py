# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""`python3 -m lamss.mission`, which `lamss mission` runs."""

from __future__ import annotations

import argparse
import fnmatch
import subprocess
import sys
from pathlib import Path
from typing import Any, Optional, Sequence

import yaml

from .. import style
from . import missiontest, render, supervisor, workspace
from .errors import MissionError
from .registry import Registry
from .resolve import Mission, resolve
from .variant import variant_target


def load_registry() -> Registry:
    return Registry(workspace.toolboxes(workspace.find_root())).load()


def _yaml(data: Any) -> str:
    return yaml.safe_dump(data, sort_keys=False, default_flow_style=False, allow_unicode=True)


def _mission_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("cruise", help="cruise name (see `list`)")
    parser.add_argument("variant", nargs="?", help="variant name; combinations are variants of their own")
    parser.add_argument("--set", dest="sets", metavar="KEY=VALUE", action="append", default=[],
                        help="override a parameter, e.g. warp=10 or platform.auv.max_depth=50 (repeatable)")


def _resolve(args: argparse.Namespace, mode: str) -> Mission:
    mission = resolve(load_registry(), args.cruise, args.variant, mode, args.sets)
    for note in mission.notes:
        style.print_note(note)
    return mission


def _inline(value: Any) -> str:
    return yaml.safe_dump([value], default_flow_style=True, width=1 << 16).strip()[1:-1]


def explain(mission: Mission, key: str) -> str:
    value, layers = mission.explain(key)
    lines = [style.heading(key) + f" = {_inline(value)}"]
    width = max((len(e.source) for e in layers), default=0)
    for e in layers:
        mark = style.good("  <- wins") if e.wins else ""
        lines.append(f"  {e.kind:<8}  {e.source:<{width}}  {e.key} = "
                     f"{_inline(e.value)}{mark}")
    return "\n".join(lines)


def explain_brief(mission: Mission, keys: Sequence[str]) -> str:
    """One line per key: its value and the source that wins."""
    rows = []
    for key in keys:
        value, layers = mission.explain(key)
        if key in mission.nodes or f"cruise.{key}" in mission.nodes:
            source = "; ".join(f"{e.kind} {e.source}" for e in layers)
        else:
            winner = next((e for e in reversed(layers) if e.wins), layers[-1])
            source = f"{winner.kind}  {winner.source}"
        rows.append((key, _inline(value), source))
    width = max((len(k) + len(v) for k, v, _ in rows), default=0) + 3
    # pad the plain text first, so colour codes don't upset the alignment
    return "\n".join(style.heading(k) + f"{f'{k} = {v}':<{width}}"[len(k):] + style.note(f"  <- {w}")
                     for k, v, w in rows)


def explanations(mission: Mission, patterns: Sequence[str], everything: bool) -> str:
    """Every layer for a plain key; one line per match for a pattern (`*`, `?`, `[...]`)."""
    if everything:
        return explain_brief(mission, mission.keys())
    out = []
    for pattern in patterns:
        if any(c in pattern for c in "*?["):
            matches = [k for k in mission.keys() if fnmatch.fnmatchcase(k, pattern)]
            if not matches:
                raise MissionError(f"no key matches {pattern!r}")
            out.append(explain_brief(mission, matches))
        else:
            out.append(explain(mission, pattern))
    return "\n".join(out)


def cmd_list(args: argparse.Namespace) -> int:
    registry = load_registry()
    cruises = registry.cruises()
    if args.cruise:
        mission = resolve(registry, args.cruise, None, "sim")
        for note in mission.notes:
            style.print_note(note)
        print(style.heading(mission.name)
              + style.note(f" ({registry.toolbox_of(type(mission.cruise)).name})") + ": "
              f"fleet {mission.fleet.name}"
              + (f", environment {mission.environment.name}" if mission.environment else ""))
        for p in mission.platforms:
            for s in p.resolved_subsystems:
                print(f"  {p.name}/{s.name}: {', '.join(c.name for c in s.components) or '-'}")
        names = _applicable_variants(registry, mission)
        print(f"  {style.heading('variants:')} {', '.join(names) or '-'}")
        return 0
    if not cruises:
        print(style.attention("no cruises in the active toolboxes "
              f"({', '.join(t.name for t in registry.toolboxes) or 'none have a missions/ tree'})"))
        return 0
    width = max(len(n) for n in cruises)
    for name, cls in sorted(cruises.items()):
        try:
            variants = ", ".join(_applicable_variants(registry, resolve(registry, name, None, "sim")))
        except MissionError as e:
            variants = f"(cannot resolve: {e})"
        print(f"{style.heading(f'{name:<{width}}')}  "
              f"{style.note(f'{registry.toolbox_of(cls).name:<12}')}  {variants}")
    return 0


def _applicable_variants(registry: Registry, mission: Mission) -> list[str]:
    used = {type(n) for n in mission.nodes.values()}
    return sorted(name for name, classes in registry.variants().items()
                  if any(variant_target(c) in used for c in classes))


def cmd_show(args: argparse.Namespace) -> int:
    mission = _resolve(args, args.mode)
    if args.explain or args.explain_all:
        print(explanations(mission, args.explain, args.explain_all))
    else:
        print(_yaml(mission.to_dict()), end="")
    return 0


def _layout(args: argparse.Namespace, mission: Mission, suffix: Optional[str] = None) -> render.Layout:
    root = workspace.find_root()
    instance = args.instance or render.instance_name(mission.name, mission.variant, suffix)
    return render.layout_for(mission, root, workspace.prefix(root), instance)


def _generate(args: argparse.Namespace, mode: str, suffix: Optional[str] = None,
              sets: Sequence[str] = ()) -> tuple[Mission, render.Layout]:
    mission = resolve(load_registry(), args.cruise, args.variant, mode, list(args.sets) + list(sets))
    for note in mission.notes:
        style.print_note(note)
    layout = _layout(args, mission, suffix)
    if getattr(args, "func", None) in (cmd_sim, cmd_run, cmd_test) and not args.generate and supervisor.running(layout):
        raise MissionError(f"{layout.instance} is already running; `lamss mission stop {layout.instance}` first")
    if not getattr(args, "execute", False):
        render.generate(mission, layout, args.platform, args.subsystem)
        print(style.good(f"generated {layout.build}", sys.stderr), file=sys.stderr)
    elif not layout.supervisord_conf(mode).exists():
        raise MissionError(f"--execute: {layout.supervisord_conf(mode)} has not been generated")
    return mission, layout


def cmd_generate(args: argparse.Namespace) -> int:
    if args.explain:
        mission = _resolve(args, args.mode)
        print("\n".join(explain(mission, key) for key in args.explain))
        return 0
    _generate(args, args.mode)
    return 0


def launch(args: argparse.Namespace, mission: Mission, layout: render.Layout) -> int:
    selected = render.select(mission, args.platform, args.subsystem)
    supervisor.prepare(layout, [(s.platform.name, s.subsystem.name) for s in selected])
    url = supervisor.record_http_url(layout, mission.to_dict()["cruise"])
    conf = layout.supervisord_conf(mission.mode)
    if args.foreground:
        print(f"  web:     {style.command(url)}", flush=True)
        return supervisor.run_foreground(layout, conf, args.wait)
    programs = supervisor.start(layout, conf, args.wait)
    print(style.good(f"{layout.instance}: {len(programs)} processes RUNNING"))
    print(f"  web:     {style.command(url)}")
    print(f"  logs:    {(layout.run / 'log').resolve()}")
    print(f"  status:  {style.command(f'lamss mission status {layout.instance}')}")
    print(f"  stop:    {style.command(f'lamss mission stop {layout.instance}')}")
    return 0


def cmd_sim(args: argparse.Namespace) -> int:
    mission, layout = _generate(args, "sim")
    return 0 if args.generate else launch(args, mission, layout)


def cmd_run(args: argparse.Namespace) -> int:
    if not args.platform:
        raise MissionError("run needs --platform")
    mission, layout = _generate(args, "run")
    return 0 if args.generate else launch(args, mission, layout)


def _running_layout(instance: Optional[str]) -> render.Layout:
    root = workspace.find_root()
    layouts = supervisor.find_running([t.missions for t in workspace.toolboxes(root)])
    if instance:
        layouts = [l for l in layouts if l.instance == instance]
        if not layouts:
            raise MissionError(f"{instance} is not running")
    if not layouts:
        raise MissionError("no mission is running")
    if len(layouts) > 1:
        raise MissionError("several missions are running; name one: "
                           + ", ".join(l.instance for l in layouts))
    return layouts[0]


def cmd_status(args: argparse.Namespace) -> int:
    root = workspace.find_root()
    layouts = supervisor.find_running([t.missions for t in workspace.toolboxes(root)])
    if args.instance:
        layouts = [l for l in layouts if l.instance == args.instance]
    if not layouts:
        print(style.attention(f"{args.instance} is not running" if args.instance
                              else "no mission is running"))
        return 3
    status = 0
    for layout in layouts:
        url = supervisor.http_url(layout)
        print(style.heading(layout.instance) + style.note(f" ({layout.run})")
              + (f" {style.command(url)}" if url else "") + ":")
        for s in supervisor.states(layout) or []:
            state = f"{s.state:<9}"
            state = style.good(state) if s.state == supervisor.GOOD else style.attention(state) \
                if s.state in ("STARTING", "STOPPING", "STOPPED") else style.error(state, sys.stdout)
            print(f"  {s.name:<40} {state} {s.description}")
            status = status if s.state == supervisor.GOOD else 3
    return status


def cmd_stop(args: argparse.Namespace) -> int:
    layout = _running_layout(args.instance)
    supervisor.stop(layout)
    print(style.good(f"stopped {layout.instance}"))
    return 0


def cmd_tail(args: argparse.Namespace) -> int:
    layout = _running_layout(args.instance)
    if not args.program:
        log = layout.run / "log" / "supervisord.log"
        cmd = ["tail", "-n", "40"] + (["-f"] if args.follow else []) + [str(log)]
        return subprocess.call(cmd)
    names = [s.name for s in supervisor.states(layout) or []]
    matches = [n for n in names if n == args.program or n.endswith(":" + args.program)
               or n.replace("_", "/", 1).startswith(args.program.replace(":", "/"))]
    if args.program in names:
        matches = [args.program]
    if len(matches) != 1:
        raise MissionError(f"'{args.program}' matches {', '.join(matches) or 'nothing'}; programs: "
                           + ", ".join(names))
    return supervisor.supervisorctl(layout, "tail", *(["-f"] if args.follow else []), matches[0],
                                    capture=False).returncode


def cmd_test(args: argparse.Namespace) -> int:
    sets = list(args.sets)
    given = {s.split("=", 1)[0].strip().removeprefix("cruise.") for s in sets}
    probe = resolve(load_registry(), args.cruise, args.variant, "sim", sets)
    # every range moves to free ports, so tests never meet other missions on this machine
    for port_range, count in missiontest.ports_needed(probe).items():
        key = "port_base" if port_range is None else f"port_ranges.{port_range}"
        if key not in given:
            sets.append(f"{key}={missiontest.free_port_base(count)}")
    args.generate = False
    mission, layout = _generate(args, "sim", "test", sets[len(args.sets):])
    selection = render.select(mission, args.platform, args.subsystem)
    sim_args = [args.cruise] + ([args.variant] if args.variant else [])
    for s in sets:
        sim_args += ["--set", s]
    for p in args.platform:
        sim_args += ["--platform", p]
    for s in args.subsystem:
        sim_args += ["--subsystem", s]
    files = [Path(f).resolve() for f in args.test_file] or missiontest.test_files(mission)
    for f in files:
        if not f.is_file():
            raise MissionError(f"no test file {f}")
    return missiontest.run(mission, layout, selection, files, sim_args, args.wait, args.goby,
                           args.goby_args)


def _selection_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--platform", action="append", default=[], metavar="P",
                        help="only this platform (repeatable)")
    parser.add_argument("--subsystem", action="append", default=[], metavar="S",
                        help="only this subsystem (repeatable)")
    parser.add_argument("--instance", metavar="NAME",
                        help="name of the output and run directories (default: <cruise>[_<variant>])")


def _launch_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--generate", action="store_true", help="only generate, do not start")
    parser.add_argument("--execute", action="store_true",
                        help="start what was generated before, without generating")
    parser.add_argument("--foreground", action="store_true",
                        help="run supervisord in the foreground until interrupted")
    parser.add_argument("--wait", type=float, default=60, metavar="SECONDS",
                        help="how long processes may take to reach RUNNING (default 60)")


def parser() -> argparse.ArgumentParser:
    p = style.ArgumentParser(prog="lamss mission",
                                description="Generate, run and test lamss3 missions.")
    sub = p.add_subparsers(dest="command", metavar="<command>", title="Commands")

    s = sub.add_parser("list", help="list the cruises and their variants, or one cruise's platforms")
    s.add_argument("cruise", nargs="?")
    s.set_defaults(func=cmd_list)

    s = sub.add_parser("show", help="print the resolved mission, or where parameters come from")
    _mission_arguments(s)
    s.add_argument("--mode", choices=("sim", "run"), default="sim")
    s.add_argument("--explain", metavar="KEY", action="append", default=[],
                   help="print where KEY's value comes from: every layer of a parameter, or why a "
                        "concept is there; a pattern (platform.*.speed) prints one line per match "
                        "(repeatable)")
    s.add_argument("--explain-all", action="store_true",
                   help="one line for every value in the mission and where it comes from")
    s.set_defaults(func=cmd_show)

    s = sub.add_parser("generate", help="write the configuration without starting anything")
    _mission_arguments(s)
    s.add_argument("--mode", choices=("sim", "run"), default="sim")
    _selection_arguments(s)
    s.add_argument("--explain", metavar="KEY", action="append", default=[],
                   help="print where KEY's value comes from instead of generating (repeatable)")
    s.set_defaults(func=cmd_generate)

    s = sub.add_parser("sim", help="generate and start a simulation of the whole fleet on this machine")
    _mission_arguments(s)
    _selection_arguments(s)
    _launch_arguments(s)
    s.set_defaults(func=cmd_sim)

    s = sub.add_parser("run", help="generate and start one platform's subsystems on this machine")
    _mission_arguments(s)
    _selection_arguments(s)
    _launch_arguments(s)
    s.set_defaults(func=cmd_run)

    s = sub.add_parser("test", help="run the cruise's goby test files against it; "
                                    "arguments after -- go to `goby test run`",
                       usage="%(prog)s [options] cruise [variant] [-- goby test run options]")
    _mission_arguments(s)
    _selection_arguments(s)
    s.add_argument("--test-file", action="append", default=[], metavar="FILE",
                   help="run this test file (repeatable; default: the cruise's test/ directory)")
    s.add_argument("--wait", type=float, default=60, metavar="SECONDS",
                   help="how long processes may take to reach RUNNING (default 60)")
    s.add_argument("--goby", default="goby", help="the goby tool (default: goby on PATH)")
    s.set_defaults(func=cmd_test, goby_args=[])

    s = sub.add_parser("status", help="show the state of every process of the running missions")
    s.add_argument("instance", nargs="?")
    s.set_defaults(func=cmd_status)

    s = sub.add_parser("stop", help="stop a running mission and all its processes")
    s.add_argument("instance", nargs="?", help="needed when more than one is running")
    s.set_defaults(func=cmd_stop)

    s = sub.add_parser("tail", help="print a process's output (or supervisord's log)")
    s.add_argument("program", nargs="?", help="e.g. gobyd or topside_main:gobyd")
    s.add_argument("-f", "--follow", action="store_true")
    s.add_argument("--instance", metavar="NAME", help="needed when more than one is running")
    s.set_defaults(func=cmd_tail)
    return p


def main(argv: Optional[Sequence[str]] = None) -> int:
    p = parser()
    argv = list(sys.argv[1:] if argv is None else argv)
    passed: list[str] = []
    if argv and argv[0] == "test" and "--" in argv:
        i = argv.index("--")
        argv, passed = argv[:i], argv[i + 1:]
    args = p.parse_args(argv)
    if passed:
        args.goby_args = passed
    if not getattr(args, "func", None):
        p.print_help()
        return 0
    try:
        return args.func(args)
    except MissionError as e:
        style.print_error("lamss mission", e)
        return 1
