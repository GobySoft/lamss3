# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Instantiates a cruise into a tree of resolved concepts and merges their parameters.

Parameter precedence, lowest first: component (and subsystem, environment) defaults, the platform
classes along the MRO, the fleet, the cruise, the variant, then `--set`. Every key of the mission
dict is addressable as a dotted path, e.g. `platform.macrura.subsystem.main.component.logging.level`;
a key whose first part is not cruise, fleet, environment or platform is a cruise parameter.
"""

from __future__ import annotations

import inspect
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Iterable, Optional, Sequence

import yaml

from .concept import (MODES, Component, Concept, Cruise, Environment, Fleet, OptionalRef, Platform,
                      Subsystem, spec_class)
from .errors import MissionError
from .params import Explanation, Params
from .registry import Registry
from .variant import is_variant, variant_target

OWN_KIND = {"component": "default", "subsystem": "default", "environment": "default",
            "platform": "platform", "fleet": "fleet", "cruise": "cruise"}
ROOTS = ("cruise", "fleet", "environment", "platform")
RESERVED_KEYS = ("name", "subsystem", "component")


def parse_set(assignment: str) -> tuple[str, Any]:
    key, eq, text = assignment.partition("=")
    if not eq or not key.strip():
        raise MissionError(f"--set expects key=value, not {assignment!r}")
    try:
        value = yaml.safe_load(text) if text.strip() else ""
    except yaml.YAMLError:
        value = text
    return key.strip(), value


@dataclass(frozen=True)
class Origin:
    """Why a concept is in the mission: its class, the list that named it, and the class a variant
    put it in place of."""

    cls: type
    listed: Optional[str]
    spec_name: Optional[str] = None
    replaced: Optional[type] = None


@dataclass
class Mission:
    registry: Registry
    mode: str
    variant: Optional[str]
    cruise: Cruise
    fleet: Fleet
    environment: Optional[Environment]
    platforms: list[Platform]
    notes: list[str] = field(default_factory=list)
    nodes: dict[str, Concept] = field(default_factory=dict)
    params: dict[str, Params] = field(default_factory=dict)
    origins: dict[str, Origin] = field(default_factory=dict)

    @property
    def name(self) -> str:
        return self.cruise.name

    def platform(self, name: str) -> Platform:
        for p in self.platforms:
            if p.name == name:
                return p
        raise MissionError(f"no platform '{name}' in cruise {self.name} "
                           f"(platforms: {', '.join(p.name for p in self.platforms)})")

    def to_dict(self) -> dict[str, Any]:
        return {
            "mode": self.mode,
            "variant": self.variant,
            "cruise": self.cruise.to_dict(),
            "fleet": self.fleet.to_dict(),
            "environment": self.environment.to_dict() if self.environment else None,
            "platform": {p.name: p.to_dict() for p in self.platforms},
        }

    def route(self, key: str) -> tuple[str, str]:
        """The node a dotted key belongs to, and the parameter key within it."""
        parts = key.split(".")
        if parts[0] not in ROOTS:
            parts = ["cruise"] + parts
        for i in range(len(parts) - 1, 0, -1):
            node = ".".join(parts[:i])
            if node in self.params:
                return node, ".".join(parts[i:])
        if ".".join(parts) in self.params:
            known = ", ".join(self.params[".".join(parts)].flat()) or "none"
            raise MissionError(f"'{key}' is a {self.nodes['.'.join(parts)].kind}, not a parameter; "
                               f"its parameters: {known}")
        prefix = ".".join(parts[:2]) if parts[0] == "platform" else parts[0]
        children = sorted(k for k in self.params if k.startswith(prefix.rsplit(".", 1)[0] + "."))
        raise MissionError(f"'{key}' names nothing in cruise {self.name}"
                           + (f"; did you mean one of: {', '.join(children)}" if children else ""))

    def explain(self, key: str) -> tuple[Any, list[Explanation]]:
        """The value of any key `show` prints, and where it comes from: a parameter's layers, the
        class and list behind a concept, a derived value's method, or the command line."""
        if key in ("mode", "variant"):
            value = getattr(self, key)
            return value, [Explanation(key, "command", COMMAND_SOURCES[key], value, True)]
        full = key if key.split(".")[0] in ROOTS else f"cruise.{key}"
        if full in self.nodes:
            return type(self.nodes[full]).__name__, self._explain_concept(full)
        if full == "environment" and self.environment is None:
            source = (f"{type(self.cruise).__name__}.environment" if self.mode == "sim"
                      else "run mode, which has no environment")
            return None, [Explanation(key, "cruise", source, None, True)]
        node, param = self.route(key)
        concept = self.nodes[node]
        flat = self.params[node].flat()
        allocated = self._explain_allocated_port(node, param, flat)
        if allocated:
            return allocated
        if param == "name":
            return concept.name, [self._explain_name(node)]
        if any(k == param or k.startswith(param + ".") for k in flat):
            value = flat[param] if param in flat else _lookup(concept.params, param)
            return value, self.params[node].explain(param)
        derived = concept.derived()
        if param.split(".")[0] in derived:
            value = _lookup(derived, param)
            owner = next(c for c in type(concept).__mro__ if "derived" in c.__dict__)
            return value, [Explanation(param, "derived", f"{owner.__name__}.derived()", value, True)]
        if concept.kind == "fleet" and param == "platforms":
            value = [p.name for p in self.platforms]
            return value, [Explanation(param, "listed", f"{self._where(type(concept))}.platforms",
                                       value, True)]
        raise MissionError(f"{node} has no value '{param}' (parameters: {', '.join(flat) or 'none'}"
                           + (f"; derived: {', '.join(derived)}" if derived else "") + ")")

    def keys(self) -> list[str]:
        """Every key `explain` accepts, in the order `show` prints them: each concept, then its
        values."""
        out = ["mode", "variant"]

        def walk(prefix: str, values: dict[str, Any]) -> None:
            for k, v in values.items():
                key = f"{prefix}.{k}" if prefix else k
                if key in self.nodes:
                    out.append(key)
                    walk(key, v)
                elif isinstance(v, dict) and v:
                    walk(key, v)
                else:
                    out.append(key)

        walk("", {k: v for k, v in self.to_dict().items() if k not in ("mode", "variant")})
        return out

    def _where(self, cls: type) -> str:
        path, toolbox = self.registry.path_of(cls), self.registry.toolbox_of(cls)
        if path and toolbox:
            return f"{cls.__name__} ({toolbox.name}:{path})"
        try:
            file = Path(inspect.getfile(cls)).resolve()
        except TypeError:
            return cls.__name__
        for t in self.registry.toolboxes:
            if file.is_relative_to(t.missions):
                return f"{cls.__name__} ({t.name}:{file.relative_to(t.missions).with_suffix('')})"
        return cls.__name__

    def _explain_concept(self, key: str) -> list[Explanation]:
        origin, value = self.origins[key], type(self.nodes[key]).__name__
        out = [Explanation(key, "listed", origin.listed or "the command line", value, False)]
        if origin.replaced is not None:
            out.append(Explanation(key, "variant", f"{origin.cls.__name__} replaces "
                                   f"{origin.replaced.__name__} (variant {self.variant})", value, False))
        out.append(Explanation(key, "class", self._where(origin.cls), value, True))
        return out

    def _explain_name(self, key: str) -> Explanation:
        origin, name = self.origins[key], self.nodes[key].name
        if origin.spec_name:
            source = f"{origin.listed}: {origin.cls.__name__}({origin.spec_name!r})"
        else:
            source = f"the name of {self._where(origin.replaced or origin.cls)}"
        return Explanation("name", "name", source, name, True)

    def _explain_allocated_port(self, node: str, param: str, flat: dict[str, Any]):
        concept = self.nodes[node]
        if not param.startswith("ports.") or param in flat or concept.kind != "subsystem":
            return None
        name = param[len("ports."):]
        port = getattr(concept, "allocated_ports", {}).get(name)
        if port is None:
            return None
        port_range = next((r for c in concept.components for n, r in component_ports(c).items()
                           if n == name), None)
        source = "cruise.port_base" if port_range is None else f"cruise.port_ranges.{port_range}"
        return port, [Explanation(param, "allocated", source, port, True)]


COMMAND_SOURCES = {"mode": "the command (sim or run; show --mode)",
                   "variant": "the command (lamss mission <command> <cruise> [variant])"}


def _lookup(values: dict[str, Any], dotted: str) -> Any:
    for part in dotted.split("."):
        values = values[part]
    return values


class _Resolver:
    def __init__(self, registry: Registry, mode: str, variant: Optional[str]):
        if mode not in MODES:
            raise MissionError(f"mode must be one of {MODES}, not {mode!r}")
        self.registry = registry
        self.mode = mode
        self.variant = variant
        self.substitutions: dict[type, type] = {}
        if variant:
            classes = registry.variants(variant)[variant]
            if not classes:
                known = ", ".join(sorted(registry.variants())) or "none"
                raise MissionError(f"no variant named '{variant}' (variants: {known})")
            self.substitutions = {variant_target(c): c for c in classes}
        self.used: set[type] = set()
        self.notes: list[str] = []
        self.nodes: dict[str, Concept] = {}
        self.params: dict[str, Params] = {}
        self.origins: dict[str, Origin] = {}
        self.replaced: dict[type, type] = {}

    def lookup(self, entry: Any, base: type, where: str) -> Optional[type]:
        if isinstance(entry, OptionalRef):
            cls = self.registry.find(entry.ref)
            if cls is None:
                self.notes.append(f"{where}: optional component '{entry.ref}' left out "
                                  f"(no active toolbox defines it)")
                return None
        elif isinstance(entry, str):
            cls = self.registry.get(entry, base)
        else:
            cls = spec_class(entry)
        if cls is None or not issubclass(cls, base):
            raise MissionError(f"{where}: {entry!r} is not a {base.kind}")
        self.used.add(cls)
        substitute = self.substitutions.get(cls, cls)
        if substitute is not cls:
            self.replaced[substitute] = cls
        return substitute

    def node(self, cls: type, entry: Any, origin: Optional[tuple[type, str]], key: str,
             name: str, parent: Optional[Concept]) -> Concept:
        node = cls.__new__(cls)
        node._resolved = True
        node.name = name
        node.path = key
        node.mode = self.mode
        node.parent = parent
        params = Params(key)
        own_kind = OWN_KIND[cls.kind]
        for c in reversed(cls.__mro__):
            own = c.__dict__.get("params")
            if isinstance(own, dict) and own:
                params.add("variant" if is_variant(c) else own_kind, f"{c.__name__}.params", own)
        if isinstance(entry, Concept) and entry._spec_overrides and origin:
            owner, attr = origin
            params.add(self.kind_of(owner), f"{owner.__name__}.{attr}", entry._spec_overrides,
                       strict=True)
        self.nodes[key] = node
        self.params[key] = params
        self.origins[key] = Origin(cls, f"{origin[0].__name__}.{origin[1]}" if origin else None,
                                   entry._spec_name if isinstance(entry, Concept) else None,
                                   self.replaced.get(cls))
        return node

    def kind_of(self, owner: type) -> str:
        return "variant" if is_variant(owner) else OWN_KIND[owner.kind]

    @staticmethod
    def origin(owner: type, attr: str, entry: Any) -> tuple[type, str]:
        """The most basic class in `owner`'s MRO whose own `attr` lists this very entry, so an
        entry inherited by a variant keeps its original precedence."""
        for c in reversed(owner.__mro__):
            value = c.__dict__.get(attr)
            if value is entry or (isinstance(value, (list, tuple)) and any(e is entry for e in value)):
                return c, attr
        return owner, attr

    def name_of(self, cls: type, entry: Any) -> str:
        if isinstance(entry, Concept) and entry._spec_name:
            return entry._spec_name
        if issubclass(cls, Component):
            name = self.registry.component_name(cls)
        else:
            name = self.registry.concept_name(cls)
        return name or variant_target(cls).__name__.lower()

    def resolve(self, cruise_name: str) -> Mission:
        cruise_cls = self.lookup(cruise_name, Cruise, "cruise")
        assert cruise_cls is not None
        cruise = self.node(cruise_cls, None, None, "cruise", self.name_of(cruise_cls, None), None)

        fleet_entry = cruise_cls.fleet
        if fleet_entry is None:
            raise MissionError(f"cruise {cruise.name} ({cruise_cls.__name__}) sets no fleet")
        fleet_cls = self.lookup(fleet_entry, Fleet, f"cruise {cruise.name}")
        fleet = self.node(fleet_cls, fleet_entry, self.origin(cruise_cls, "fleet", fleet_entry),
                          "fleet", self.name_of(fleet_cls, fleet_entry), cruise)

        environment = None
        env_entry = cruise_cls.environment
        if self.mode == "sim" and env_entry is not None:
            env_cls = self.lookup(env_entry, Environment, f"cruise {cruise.name}")
            environment = self.node(env_cls, env_entry, self.origin(cruise_cls, "environment", env_entry),
                                    "environment", self.name_of(env_cls, env_entry), cruise)

        platforms = []
        for entry in fleet_cls.platforms:
            cls = self.lookup(entry, Platform, f"fleet {fleet.name}")
            name = self.name_of(cls, entry)
            key = f"platform.{name}"
            if key in self.nodes:
                raise MissionError(f"fleet {fleet.name} has two platforms named '{name}'; "
                                   f"name one, e.g. {cls.__name__}('{name}2')")
            platform = self.node(cls, entry, self.origin(fleet_cls, "platforms", entry), key, name, fleet)
            platform.resolved_subsystems = self.subsystems(platform, environment)
            platforms.append(platform)
        fleet.resolved_platforms = platforms
        if not platforms:
            raise MissionError(f"fleet {fleet.name} has no platforms")

        unused = [c for c in self.substitutions if c not in self.used]
        if self.variant and len(unused) == len(self.substitutions):
            changes = ", ".join(sorted(self.registry.path_of(c) or c.__name__ for c in unused))
            raise MissionError(f"variant '{self.variant}' does not apply to cruise {cruise.name} "
                               f"(it changes {changes})")

        mission = Mission(self.registry, self.mode, self.variant, cruise, fleet, environment,
                          platforms, self.notes, self.nodes, self.params, self.origins)
        for owner, node_kind in ((cruise_cls, "cruise"), (fleet_cls, "fleet")):
            for c in reversed(owner.__mro__):
                overrides = c.__dict__.get("overrides")
                if isinstance(overrides, dict):
                    self.route_layer(mission, self.kind_of(c), f"{c.__name__}.overrides", overrides)
        return mission

    def route_layer(self, mission: Mission, kind: str, source: str, overrides: dict[str, Any]) -> None:
        for key, value in overrides.items():
            if key.split(".")[0] == "environment" and mission.environment is None:
                continue
            node, param = mission.route(key)
            self.params[node].add(kind, source, {param: value}, dotted=True, strict=True)

    def subsystems(self, platform: Platform, environment: Optional[Environment]) -> list[Subsystem]:
        cls = type(platform)
        out = []
        entries = list(cls.subsystems)
        names = [e._spec_name if isinstance(e, Subsystem) else None for e in entries]
        if None in names:
            raise MissionError(f"{cls.__name__}.subsystems must list Subsystem('<name>', ...) entries")
        main_entries = [(e, self.origin(cls, a, e)) for a in ("components", f"{self.mode}_components")
                        for e in getattr(cls, a)]
        if main_entries and cls.main_subsystem not in names:
            raise MissionError(f"{cls.__name__} lists components but has no '{cls.main_subsystem}' subsystem")
        for entry in entries:
            if entry._spec_mode not in (None, self.mode):
                continue
            key = f"{platform.path}.subsystem.{entry._spec_name}"
            if key in self.nodes:
                raise MissionError(f"{cls.__name__} has two subsystems named '{entry._spec_name}'")
            sub_cls = type(entry)
            subsystem = self.node(sub_cls, entry, self.origin(cls, "subsystems", entry), key,
                                  entry._spec_name, platform)
            subsystem.platform = platform
            sub_origin = self.origin(cls, "subsystems", entry)
            component_entries = [(e, (sub_origin[0], f"subsystems[{entry._spec_name}]"))
                                 for e in entry._spec_components]
            if entry._spec_name == cls.main_subsystem:
                component_entries += main_entries
            if environment is not None and entry._spec_name == type(environment).subsystem:
                component_entries += [(e, self.origin(type(environment), "components", e))
                                      for e in type(environment).components]
            subsystem.components = self.components(subsystem, component_entries)
            out.append(subsystem)
        return out

    def components(self, subsystem: Subsystem, entries: Sequence[tuple[Any, tuple[type, str]]]) -> list[Component]:
        where = subsystem.path
        out: list[Component] = []
        for entry, origin in entries:
            cls = self.lookup(entry, Component, where)
            if cls is None or (cls.sim_only and self.mode != "sim") or (cls.run_only and self.mode != "run"):
                continue
            if any(type(c) is cls for c in out):
                raise MissionError(f"{where} lists {cls.__name__} twice")
            name = self.name_of(cls, entry)
            key = f"{where}.component.{name}"
            if key in self.nodes:
                raise MissionError(f"{where} has two components named '{name}' "
                                   f"({type(self.nodes[key]).__name__} and {cls.__name__})")
            component = self.node(cls, entry, origin, key, name, subsystem)
            component.subsystem = subsystem
            out.append(component)
        for component in out:
            for req in type(component).requires:
                req_cls = self.registry.find(req) if isinstance(req, str) else req
                if req_cls is None:
                    raise MissionError(f"{component.path} requires '{req}', which no active toolbox defines")
                if not any(isinstance(c, req_cls) for c in out):
                    raise MissionError(f"{component.path} requires {req_cls.__name__}, which "
                                       f"{where} does not have")
        return out


def _finish(mission: Mission) -> None:
    for key, node in mission.nodes.items():
        node.params = mission.params[key].merged()  # type: ignore[misc]
        reserved = [k for k in RESERVED_KEYS if k in node.params]
        if reserved:
            raise MissionError(f"{key}: '{reserved[0]}' is reserved and cannot be a parameter")
    modem_ids: dict[Any, str] = {}
    for p in mission.platforms:
        modem_id = p.params.get("modem_id")
        if modem_id is not None:
            if modem_id in modem_ids:
                raise MissionError(f"platforms {modem_ids[modem_id]} and {p.name} both have modem_id {modem_id}")
            modem_ids[modem_id] = p.name
    _allocate_ports(mission)


def component_ports(component: Component) -> dict[str, Optional[str]]:
    """{port name: range name, or None for cruise.port_base} for the ports a component needs."""
    ports = type(component).ports
    return dict(ports) if isinstance(ports, dict) else {name: None for name in ports}


def _allocate_ports(mission: Mission) -> None:
    subsystems = [s for p in mission.platforms for s in p.resolved_subsystems]
    used = {v for s in subsystems for v in s.params["ports"].values()}
    params = mission.cruise.params
    next_port = {None: int(params["port_base"])}
    for name, base in (params.get("port_ranges") or {}).items():
        next_port[name] = int(base)
    for s in subsystems:
        ports = dict(s.params["ports"])
        owners: dict[str, str] = {}
        for c in s.components:
            for name, port_range in component_ports(c).items():
                if name in owners:
                    raise MissionError(f"{s.path}: {owners[name]} and {c.path} both need port '{name}'")
                owners[name] = c.path
                if port_range not in next_port:
                    raise MissionError(f"{c.path}: port '{name}' uses range '{port_range}', which "
                                       "cruise.port_ranges does not define")
                if name not in ports:
                    while next_port[port_range] in used:
                        next_port[port_range] += 1
                    ports[name] = next_port[port_range]
                    used.add(next_port[port_range])
        s.allocated_ports = ports


def resolve(registry: Registry, cruise: str, variant: Optional[str] = None, mode: str = "sim",
            sets: Iterable[str | tuple[str, Any]] = ()) -> Mission:
    """The resolved mission for `cruise` with `variant` applied and `sets` (key=value) on top."""
    resolver = _Resolver(registry, mode, variant)
    mission = resolver.resolve(cruise)
    overrides: dict[str, Any] = {}
    for s in sets:
        key, value = parse_set(s) if isinstance(s, str) else s
        overrides[key] = value
    for key, value in overrides.items():
        node, param = mission.route(key)
        mission.params[node].add("set", f"--set {key}", {param: value}, dotted=True, strict=True)
    _finish(mission)
    return mission
