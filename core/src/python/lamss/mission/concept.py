# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""The concept classes that mission trees subclass.

Mission classes are definitions: class attributes say what a cruise, platform or component is.
Calling a class with keyword arguments makes a spec that overrides parameters where it is listed,
e.g. ``components = [Logging(level="DEBUG")]``. `resolve` turns the classes into a tree of
resolved instances, one per use, whose `params` hold the merged values.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, ClassVar, Optional as Opt, Sequence, Union

from .errors import MissionError

MODES = ("sim", "run")


class Concept:
    kind: ClassVar[str] = "concept"
    params: ClassVar[dict[str, Any]] = {}

    def __init__(self, name: Opt[str] = None, **overrides: Any):
        self._spec_name = name
        self._spec_overrides = overrides
        self._resolved = False

    # set on resolved instances by lamss.mission.resolve
    name: str
    path: str
    mode: str
    parent: Opt["Concept"]

    def to_dict(self) -> dict[str, Any]:
        """The merged parameters plus the children's dicts; all that templates see of a concept."""
        out: dict[str, Any] = {"name": self.name}
        out.update(self.params)  # type: ignore[arg-type]
        out.update(self.derived())
        return out

    def derived(self) -> dict[str, Any]:
        """Values computed from the merged parameters (lookups, arithmetic) for the templates."""
        return {}

    def __repr__(self) -> str:
        if getattr(self, "_resolved", False):
            return f"<{type(self).__name__} {self.path}>"
        args = ([repr(self._spec_name)] if self._spec_name else []) + [
            f"{k}={v!r}" for k, v in self._spec_overrides.items()]
        return f"{type(self).__name__}({', '.join(args)})"

    def __eq__(self, other: object) -> bool:
        if getattr(self, "_resolved", False) or not isinstance(other, Concept):
            return self is other
        return (type(self) is type(other) and self._spec_name == other._spec_name
                and self._spec_overrides == other._spec_overrides
                and _spec_extra(self) == _spec_extra(other))

    def __hash__(self) -> int:
        return id(self)


def _spec_extra(spec: Concept) -> Any:
    if isinstance(spec, Subsystem):
        return (spec._spec_components, spec._spec_mode)
    return None


@dataclass(frozen=True)
class Process:
    """One supervisord program. `command` is a Jinja2 template rendered with the context."""

    name: str
    command: str
    config_template: Opt[str] = None
    priority: Opt[int] = None
    autorestart: Union[bool, str] = "unexpected"
    startsecs: int = 1
    stopsignal: str = "TERM"
    environment: dict[str, str] = field(default_factory=dict)
    # answers Goby health requests, so a health monitor can expect it
    health: bool = False


@dataclass(frozen=True)
class OptionalRef:
    """A component that is left out, with a note, when no active toolbox defines it."""

    ref: str


def optional(ref: str) -> OptionalRef:
    return OptionalRef(ref)


ComponentEntry = Union[type["Component"], "Component", str, OptionalRef]


class Component(Concept):
    """Contributes processes, configuration templates and parameters to a subsystem."""

    kind = "component"
    requires: ClassVar[Sequence[Union[type["Component"], str]]] = []
    sim_only: ClassVar[bool] = False
    run_only: ClassVar[bool] = False
    processes: ClassVar[Sequence[Process]] = []
    templates: ClassVar[Sequence[str]] = []
    data_files: ClassVar[Sequence[str]] = []
    # templates rendered into a file another component owns, e.g. {"last.moos": ["pApp.moos.j2"]};
    # the owner's template sees them as fragments["last.moos"]
    fragments: ClassVar[dict[str, Sequence[str]]] = {}
    # TCP/UDP ports this component needs, allocated per subsystem unless the subsystem's `ports`
    # parameter sets them: a list of names (from cruise.port_base) or {name: range} to draw from
    # cruise.port_ranges[range], e.g. {"liaison": "http"} keeps web pages away from MOOS's 9000s
    ports: ClassVar[Union[Sequence[str], dict[str, Opt[str]]]] = []

    subsystem: "Subsystem"

    def environment(self, paths: dict[str, Any]) -> dict[str, str]:
        """Variables for this subsystem's processes and its env.sh; `paths` is the context's."""
        return {}

    def test_endpoint(self, paths: dict[str, Any]) -> Opt[dict[str, Any]]:
        """`goby test` endpoint settings for this subsystem (e.g. {"zeromq": {...}}), if any."""
        return None

    def active_processes(self) -> list[Process]:
        """The processes to launch; override to make them depend on the parameters or mode."""
        return list(self.processes)

    def active_templates(self) -> list[str]:
        return list(self.templates)

    def active_fragments(self) -> dict[str, list[str]]:
        return {target: list(names) for target, names in self.fragments.items()}

    @property
    def platform(self) -> "Platform":
        return self.subsystem.platform


class Subsystem(Concept):
    """Part of a platform with its own processes and supervisord fragment (often its own computer)."""

    kind = "subsystem"
    params = dict(host="localhost", ports={})

    def __init__(self, name: str, components: Sequence[ComponentEntry] = (),
                 mode: Opt[str] = None, **overrides: Any):
        if not name:
            raise MissionError("a Subsystem needs a name")
        if mode is not None and mode not in MODES:
            raise MissionError(f"Subsystem {name!r}: mode must be one of {MODES}, not {mode!r}")
        super().__init__(name, **overrides)
        self._spec_components = list(components)
        self._spec_mode = mode

    platform: "Platform"
    components: list[Component]
    allocated_ports: dict[str, int]

    def derived(self) -> dict[str, Any]:
        return {"ports": dict(self.allocated_ports)}

    def to_dict(self) -> dict[str, Any]:
        out = super().to_dict()
        out["component"] = {c.name: c.to_dict() for c in self.components}
        return out


class Platform(Concept):
    """A vehicle, ship, mooring or topside station. `components` go to the `main_subsystem`."""

    kind = "platform"
    params = dict(type="platform", modem_id=None)
    components: ClassVar[Sequence[ComponentEntry]] = []
    sim_components: ClassVar[Sequence[ComponentEntry]] = []
    run_components: ClassVar[Sequence[ComponentEntry]] = []
    subsystems: ClassVar[Sequence[Subsystem]] = [Subsystem("main")]
    main_subsystem: ClassVar[str] = "main"

    resolved_subsystems: list[Subsystem]

    def to_dict(self) -> dict[str, Any]:
        out = super().to_dict()
        out["subsystem"] = {s.name: s.to_dict() for s in self.resolved_subsystems}
        return out


class Environment(Concept):
    """Bathymetry, sound speed, currents, ice and time window; only instantiated in simulation."""

    kind = "environment"
    # simulators of this environment, added to every platform subsystem named `subsystem`
    components: ClassVar[Sequence[ComponentEntry]] = []
    subsystem: ClassVar[str] = "sensor_sim"


class Fleet(Concept):
    kind = "fleet"
    platforms: ClassVar[Sequence[Union[type[Platform], Platform, str]]] = []
    # dotted keys into the mission, e.g. {"platform.macrura.modem_id": 4}
    overrides: ClassVar[dict[str, Any]] = {}

    resolved_platforms: list[Platform]

    def to_dict(self) -> dict[str, Any]:
        out = super().to_dict()
        out["platforms"] = [p.name for p in self.resolved_platforms]
        return out


class Cruise(Concept):
    """Bounds a deployment: datum, region, acomms network, fleet and environment."""

    kind = "cruise"
    params = dict(datum=dict(lat=0.0, lon=0.0), warp=1, port_base=9000,
                  port_ranges=dict(http=50000, supervisor=51000),
                  supervisor=dict(http_port=None, http_address="127.0.0.1"))
    fleet: ClassVar[Union[type[Fleet], Fleet, str, None]] = None
    environment: ClassVar[Union[type[Environment], Environment, str, None]] = None
    overrides: ClassVar[dict[str, Any]] = {}


BASE_CLASSES: dict[str, type[Concept]] = {
    "cruise": Cruise,
    "fleet": Fleet,
    "platform": Platform,
    "subsystem": Subsystem,
    "component": Component,
    "environment": Environment,
}


def spec_class(entry: Any) -> Opt[type]:
    if isinstance(entry, type):
        return entry
    if isinstance(entry, Concept):
        return type(entry)
    return None


def entry_key(entry: Any) -> Any:
    """Identifies an entry of a component, platform or subsystem list across variants."""
    if isinstance(entry, Subsystem):
        return ("subsystem", entry._spec_name)
    if isinstance(entry, Concept):
        return (type(entry), entry._spec_name)
    if isinstance(entry, type):
        return (entry, None)
    if isinstance(entry, OptionalRef):
        return ("optional", entry.ref)
    if isinstance(entry, Process):
        return ("process", entry.name)
    return ("value", entry)


def replace(entries: Sequence[Any], old: Any, new: Any) -> list[Any]:
    """`entries` with `new` in the place of `old` (a class, spec or name)."""
    old_key = entry_key(old)
    out = list(entries)
    for i, entry in enumerate(out):
        if entry_key(entry) == old_key or (isinstance(old, type) and spec_class(entry) is old):
            out[i] = new
            return out
    raise MissionError(f"replace: {old!r} is not in {out!r}")


def remove(entries: Sequence[Any], old: Any) -> list[Any]:
    """`entries` without `old` (a class, spec or name)."""
    old_key = entry_key(old)
    out = [e for e in entries
           if not (entry_key(e) == old_key or (isinstance(old, type) and spec_class(e) is old))]
    if len(out) == len(entries):
        raise MissionError(f"remove: {old!r} is not in {list(entries)!r}")
    return out
