# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""`python3 -m lamss.mission`, which `lamss mission` runs."""

from __future__ import annotations

import argparse
import sys
from typing import Any, Optional, Sequence

import yaml

from . import workspace
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
        print(f"note: {note}", file=sys.stderr)
    return mission


def _inline(value: Any) -> str:
    return yaml.safe_dump([value], default_flow_style=True, width=1 << 16).strip()[1:-1]


def explain(mission: Mission, key: str) -> str:
    value, layers = mission.explain(key)
    lines = [f"{key} = {_inline(value)}"]
    width = max((len(e.source) for e in layers), default=0)
    for e in layers:
        mark = "  <- wins" if e.wins else ""
        lines.append(f"  {e.kind:<8}  {e.source:<{width}}  {e.key} = "
                     f"{_inline(e.value)}{mark}")
    return "\n".join(lines)


def cmd_list(args: argparse.Namespace) -> int:
    registry = load_registry()
    cruises = registry.cruises()
    if args.cruise:
        mission = resolve(registry, args.cruise, None, "sim")
        for note in mission.notes:
            print(f"note: {note}", file=sys.stderr)
        print(f"{mission.name} ({registry.toolbox_of(type(mission.cruise)).name}): "
              f"fleet {mission.fleet.name}"
              + (f", environment {mission.environment.name}" if mission.environment else ""))
        for p in mission.platforms:
            for s in p.resolved_subsystems:
                print(f"  {p.name}/{s.name}: {', '.join(c.name for c in s.components) or '-'}")
        names = _applicable_variants(registry, mission)
        print(f"  variants: {', '.join(names) or '-'}")
        return 0
    if not cruises:
        print("no cruises in the active toolboxes "
              f"({', '.join(t.name for t in registry.toolboxes) or 'none have a missions/ tree'})")
        return 0
    width = max(len(n) for n in cruises)
    for name, cls in sorted(cruises.items()):
        try:
            variants = ", ".join(_applicable_variants(registry, resolve(registry, name, None, "sim")))
        except MissionError as e:
            variants = f"(cannot resolve: {e})"
        print(f"{name:<{width}}  {registry.toolbox_of(cls).name:<12}  {variants}")
    return 0


def _applicable_variants(registry: Registry, mission: Mission) -> list[str]:
    used = {type(n) for n in mission.nodes.values()}
    return sorted(name for name, classes in registry.variants().items()
                  if any(variant_target(c) in used for c in classes))


def cmd_show(args: argparse.Namespace) -> int:
    mission = _resolve(args, args.mode)
    if args.explain:
        print("\n".join(explain(mission, key) for key in args.explain))
    else:
        print(_yaml(mission.to_dict()), end="")
    return 0


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(prog="lamss mission",
                                description="Generate, run and test lamss3 missions.")
    sub = p.add_subparsers(dest="command", metavar="<command>")

    s = sub.add_parser("list", help="list the cruises and their variants, or one cruise's platforms")
    s.add_argument("cruise", nargs="?")
    s.set_defaults(func=cmd_list)

    s = sub.add_parser("show", help="print the resolved mission, or where parameters come from")
    _mission_arguments(s)
    s.add_argument("--mode", choices=("sim", "run"), default="sim")
    s.add_argument("--explain", metavar="KEY", action="append", default=[],
                   help="print every layer that sets KEY and which one wins (repeatable)")
    s.set_defaults(func=cmd_show)
    return p


def main(argv: Optional[Sequence[str]] = None) -> int:
    p = parser()
    args = p.parse_args(argv)
    if not getattr(args, "func", None):
        p.print_help()
        return 0
    try:
        return args.func(args)
    except MissionError as e:
        print(f"lamss mission: error: {e}", file=sys.stderr)
        return 1
