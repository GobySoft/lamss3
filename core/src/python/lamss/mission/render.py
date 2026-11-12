# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Renders a resolved mission into its output tree.

    <toolbox>/missions/build/<instance>/
        mission.yaml                         the whole mission dict
        <platform>/<subsystem>/              context.yaml, env.sh, supervisord.conf, configs
        <mode>/supervisord.conf              includes every generated subsystem
    <toolbox>/missions/run/<instance>/       sockets, pid file; log -> ../../log/<instance>/<UTC time>

Nothing written depends on the time or on the order of directory listings, so generating twice
gives identical files.
"""

from __future__ import annotations

import hashlib
import math
import os
import shlex
import shutil
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Optional, Sequence

import jinja2
import yaml

from .concept import Component, Platform, Process, Subsystem
from .errors import MissionError
from .resolve import Mission

PACKAGE_TEMPLATES = Path(__file__).parent / "templates"
# priority = PRIORITY_STEP * (component index + 1) + process index, so earlier components start first
PRIORITY_STEP = 10
# sun_path holds 107 bytes; supervisord binds <path>.<pid> first and gobyd adds suffixes
MAX_SOCKET_PATH = 90


def pb_string(value: Any) -> str:
    """A quoted Protobuf TextFormat string."""
    text = str(value)
    for raw, escaped in (("\\", "\\\\"), ('"', '\\"'), ("\n", "\\n"), ("\t", "\\t")):
        text = text.replace(raw, escaped)
    return f'"{text}"'


def latlon(point: dict) -> str:
    """`lat: <lat> lon: <lon>` for a {lat, lon} dict, as Goby's LatLonPoint TextFormat."""
    return f"lat: {point['lat']} lon: {point['lon']}"


def utm(point: dict) -> dict:
    """The WGS84 UTM zone, hemisphere, easting and northing (m) of a {lat, lon} dict."""
    lat, lon = float(point["lat"]), float(point["lon"])
    zone = int((lon + 180) // 6) % 60 + 1
    a, f, k0 = 6378137.0, 1 / 298.257223563, 0.9996
    e2 = f * (2 - f)
    ep2 = e2 / (1 - e2)
    phi, lam = math.radians(lat), math.radians(lon)
    lam0 = math.radians((zone - 1) * 6 - 180 + 3)
    n = a / math.sqrt(1 - e2 * math.sin(phi) ** 2)
    t = math.tan(phi) ** 2
    c = ep2 * math.cos(phi) ** 2
    aa = math.cos(phi) * (lam - lam0)
    m = a * ((1 - e2 / 4 - 3 * e2 ** 2 / 64 - 5 * e2 ** 3 / 256) * phi
             - (3 * e2 / 8 + 3 * e2 ** 2 / 32 + 45 * e2 ** 3 / 1024) * math.sin(2 * phi)
             + (15 * e2 ** 2 / 256 + 45 * e2 ** 3 / 1024) * math.sin(4 * phi)
             - (35 * e2 ** 3 / 3072) * math.sin(6 * phi))
    easting = k0 * n * (aa + (1 - t + c) * aa ** 3 / 6
                        + (5 - 18 * t + t ** 2 + 72 * c - 58 * ep2) * aa ** 5 / 120) + 500000.0
    northing = k0 * (m + n * math.tan(phi) * (aa ** 2 / 2 + (5 - t + 9 * c + 4 * c ** 2) * aa ** 4 / 24
                                              + (61 - 58 * t + t ** 2 + 600 * c - 330 * ep2) * aa ** 6 / 720))
    if lat < 0:
        northing += 10000000.0
    return {"zone": zone, "hemisphere": "N" if lat >= 0 else "S",
            "easting": round(easting, 3), "northing": round(northing, 3)}


def _socket_path_filter(directory: str, name: str) -> str:
    return str(socket_path(Path(directory), name))


FILTERS = {"pb_string": pb_string, "latlon": latlon, "utm": utm, "socket_path": _socket_path_filter}


def instance_name(cruise: str, variant: Optional[str], suffix: Optional[str] = None) -> str:
    return "_".join(p for p in (cruise, variant, suffix) if p)


@dataclass(frozen=True)
class Layout:
    """Where an instance's files go."""

    root: Path
    prefix: Path
    missions: Path
    instance: str

    @property
    def build(self) -> Path:
        return self.missions / "build" / self.instance

    @property
    def run(self) -> Path:
        return self.missions / "run" / self.instance

    @property
    def logs(self) -> Path:
        return self.missions / "log" / self.instance

    @property
    def socket(self) -> Path:
        return socket_path(self.run, "supervisor.sock")

    @property
    def pidfile(self) -> Path:
        return self.run / "supervisord.pid"

    def supervisord_conf(self, mode: str) -> Path:
        return self.build / mode / "supervisord.conf"

    def subsystem_build(self, platform: str, subsystem: str) -> Path:
        return self.build / platform / subsystem

    def subsystem_run(self, platform: str, subsystem: str) -> Path:
        return self.run / platform / subsystem

    def subsystem_log(self, platform: str, subsystem: str) -> Path:
        return self.run / "log" / platform / subsystem


# written into the run directory by each launch that has no fixed supervisor.http_port
HTTP_CONF = "http.conf"


def socket_path(directory: Path, name: str) -> Path:
    """`directory/name`, or a short path derived from it when that is too long for a unix socket."""
    path = directory / name
    if len(str(path)) <= MAX_SOCKET_PATH:
        return path
    digest = hashlib.sha1(str(path).encode()).hexdigest()[:16]
    return Path(tempfile.gettempdir()) / f"lamss-{os.getuid()}" / f"{digest}_{name}"


def layout_for(mission: Mission, root: Path, prefix: Path, instance: str) -> Layout:
    toolbox = mission.registry.toolbox_of(type(mission.cruise))
    if toolbox is None:
        raise MissionError(f"cruise {mission.name} belongs to no toolbox")
    return Layout(root, prefix, toolbox.missions, instance)


@dataclass(frozen=True)
class Selected:
    platform: Platform
    subsystem: Subsystem


def select(mission: Mission, platforms: Sequence[str] = (), subsystems: Sequence[str] = ()) -> list[Selected]:
    for name in platforms:
        mission.platform(name)
    known = {s.name for p in mission.platforms for s in p.resolved_subsystems}
    for name in subsystems:
        if name not in known:
            raise MissionError(f"no subsystem '{name}' in cruise {mission.name} ({', '.join(sorted(known))})")
    out = [Selected(p, s) for p in mission.platforms if not platforms or p.name in platforms
           for s in p.resolved_subsystems if not subsystems or s.name in subsystems]
    if not out:
        raise MissionError("the --platform and --subsystem selection matches no subsystem")
    return out


def proto_libraries(root: Path, prefix: Path, toolboxes: Iterable[str]) -> list[str]:
    libs = []
    for tb in toolboxes:
        lib = prefix / "lib" / f"liblamss_{tb}_protobuf.so"
        if lib.exists():
            libs.append(str(lib))
    return libs


class Renderer:
    def __init__(self, mission: Mission, layout: Layout, selection: Sequence[Selected]):
        self.mission = mission
        self.layout = layout
        self.selection = list(selection)
        self.registry = mission.registry
        self.mission_dict = mission.to_dict()
        self.toolbox_names = [t.name for t in self.registry.toolboxes]

    def search_path(self, component: Optional[Component]) -> list[Path]:
        dirs: list[Path] = []
        for cls in type(self.mission.cruise).__mro__:
            d = self.registry.directory_of(cls)
            if d is not None:
                dirs.append(d)
        if component is not None:
            for cls in type(component).__mro__:
                d = self.registry.directory_of(cls)
                if d is not None:
                    dirs.append(d)
        dirs += [t.missions / "templates" for t in reversed(self.registry.toolboxes)]
        dirs.append(PACKAGE_TEMPLATES)
        unique: list[Path] = []
        for d in dirs:
            if d.is_dir() and d not in unique:
                unique.append(d)
        return unique

    def environment(self, component: Optional[Component]) -> jinja2.Environment:
        env = jinja2.Environment(
            loader=jinja2.FileSystemLoader([str(d) for d in self.search_path(component)]),
            undefined=jinja2.StrictUndefined, keep_trailing_newline=True, autoescape=False)
        env.filters.update(FILTERS)
        return env

    def render_template(self, name: str, context: dict, component: Optional[Component]) -> str:
        try:
            return self.environment(component).get_template(name).render(context)
        except jinja2.TemplateNotFound:
            where = ", ".join(str(d) for d in self.search_path(component))
            raise MissionError(f"template {name} not found (searched {where})") from None
        except jinja2.TemplateError as e:
            lineno = getattr(e, "lineno", None)
            owner = f" for {component.path}" if component is not None else ""
            raise MissionError(f"{name}{':' + str(lineno) if lineno else ''}{owner}: {e}") from None

    def render_string(self, text: str, context: dict, what: str) -> str:
        try:
            env = jinja2.Environment(undefined=jinja2.StrictUndefined, autoescape=False)
            env.filters.update(FILTERS)
            return env.from_string(text).render(context)
        except jinja2.TemplateError as e:
            raise MissionError(f"{what}: {e}") from None

    def paths(self, platform: Platform, subsystem: Subsystem) -> dict[str, Any]:
        lay = self.layout
        return {
            "root": str(lay.root),
            "prefix": str(lay.prefix),
            "build": str(lay.subsystem_build(platform.name, subsystem.name)),
            "run": str(lay.subsystem_run(platform.name, subsystem.name)),
            "log": str(lay.subsystem_log(platform.name, subsystem.name)),
            "data": {t.name: str(t.missions / "data") for t in self.registry.toolboxes},
            "proto_libs": proto_libraries(lay.root, lay.prefix, self.toolbox_names),
        }

    def context(self, platform: Platform, subsystem: Subsystem) -> dict[str, Any]:
        d = self.mission_dict
        platform_dict = d["platform"][platform.name]
        subsystem_dict = platform_dict["subsystem"][subsystem.name]
        return {
            "mode": d["mode"],
            "variant": d["variant"],
            "instance": self.layout.instance,
            "cruise": d["cruise"],
            "fleet": d["fleet"],
            "environment": d["environment"],
            "platform": platform_dict,
            "subsystem": subsystem_dict,
            "component": subsystem_dict["component"],
            "paths": self.paths(platform, subsystem),
            "platforms": {name: p for name, p in d["platform"].items() if name != platform.name},
        }

    def environment_variables(self, context: dict, components: Sequence[Component]) -> dict[str, str]:
        env = {
            "LAMSS_ROOT": context["paths"]["root"],
            "LAMSS_PREFIX": context["paths"]["prefix"],
            "LAMSS_INSTANCE": self.layout.instance,
            "LAMSS_CRUISE": context["cruise"]["name"],
            "LAMSS_PLATFORM": context["platform"]["name"],
            "LAMSS_SUBSYSTEM": context["subsystem"]["name"],
            "LAMSS_RUN": context["paths"]["run"],
            "LAMSS_LOG": context["paths"]["log"],
            "LAMSS_WARP": str(context["cruise"]["warp"]),
        }
        for component in components:
            for key, value in component.environment(context["paths"]).items():
                if key in env and env[key] != value:
                    raise MissionError(f"{component.path} sets {key}, which is already {env[key]!r}")
                env[key] = str(value)
        return env

    def write(self) -> list[Path]:
        build = self.layout.build
        if build.exists():
            shutil.rmtree(build)
        build.mkdir(parents=True)
        written = [_write(build / "mission.yaml", _yaml(self.mission_dict))]
        includes = []
        for sel in self.selection:
            written += self.write_subsystem(sel.platform, sel.subsystem)
            includes.append(f"../{sel.platform.name}/{sel.subsystem.name}/supervisord.conf")
        written.append(_write(self.layout.supervisord_conf(self.mission.mode),
                              self.top_conf(includes)))
        return written

    def write_subsystem(self, platform: Platform, subsystem: Subsystem) -> list[Path]:
        out = self.layout.subsystem_build(platform.name, subsystem.name)
        out.mkdir(parents=True)
        context = self.context(platform, subsystem)
        written = [_write(out / "context.yaml", _yaml(context))]

        fragments: dict[str, list[str]] = {}
        for component in subsystem.components:
            for target, names in component.active_fragments().items():
                for name in names:
                    fragments.setdefault(target, []).append(self.render_template(name, context, component))
        context = dict(context, fragments=fragments)

        files: dict[str, str] = {}
        programs: list[tuple[Component, Process, int]] = []
        for index, component in enumerate(subsystem.components):
            names = component.active_templates()
            processes = component.active_processes()
            names += [p.config_template for p in processes if p.config_template]
            for name in names:
                target = _output_name(name)
                if target in ("context.yaml", "env.sh", "supervisord.conf"):
                    raise MissionError(f"{component.path}: template {name} would overwrite {target}")
                if target in files:
                    raise MissionError(f"{subsystem.path}: two templates render {target} ({files[target]} "
                                       f"and {component.path})")
                files[target] = component.path
                written.append(_write(out / target, self.render_template(name, context, component)))
            for j, process in enumerate(processes):
                if j >= PRIORITY_STEP:
                    raise MissionError(f"{component.path} has more than {PRIORITY_STEP} processes")
                programs.append((component, process, PRIORITY_STEP * (index + 1) + j))

        env = self.environment_variables(context, subsystem.components)
        written.append(_write(out / "env.sh", env_sh(env)))
        written.append(_write(out / "supervisord.conf",
                              self.subsystem_conf(platform, subsystem, context, programs, env)))
        return written

    def subsystem_conf(self, platform: Platform, subsystem: Subsystem, context: dict,
                       programs: Sequence[tuple[Component, Process, int]], env: dict[str, str]) -> str:
        group = f"{platform.name}_{subsystem.name}"
        names = [p.name for _, p, _ in programs]
        duplicates = sorted({n for n in names if names.count(n) > 1})
        if duplicates:
            raise MissionError(f"{subsystem.path} has two processes named {duplicates[0]}")
        here_to_log = os.path.relpath(self.layout.subsystem_log(platform.name, subsystem.name),
                                      self.layout.subsystem_build(platform.name, subsystem.name))
        lines = [f"; {self.layout.instance}: {platform.name}/{subsystem.name}, generated by lamss mission",
                 "", f"[group:{group}]", f"programs={','.join(f'{group}_{n}' for n in names)}"]
        for component, process, priority in programs:
            command = self.render_string(process.command, context, f"{component.path} {process.name} command")
            program_env = dict(env, **process.environment)
            program_env["PATH"] = f"{context['paths']['prefix']}/bin:%(ENV_PATH)s"
            program_env["LD_LIBRARY_PATH"] = f"{context['paths']['prefix']}/lib"
            lines += [
                "",
                f"[program:{group}_{process.name}]",
                f"process_name={process.name}",
                f"command={_escape(command)}",
                "directory=%(here)s",
                f"priority={process.priority if process.priority is not None else priority}",
                "autostart=true",
                f"autorestart={str(process.autorestart).lower()}",
                f"startsecs={process.startsecs}",
                f"stopsignal={process.stopsignal}",
                "stopasgroup=true",
                "killasgroup=true",
                f"stdout_logfile=%(here)s/{here_to_log}/{process.name}.out",
                "redirect_stderr=true",
                f"environment={_supervisor_env(program_env)}",
            ]
        return "\n".join(lines) + "\n"

    def top_conf(self, includes: Sequence[str]) -> str:
        here_to_run = os.path.relpath(self.layout.run, self.layout.supervisord_conf(self.mission.mode).parent)
        run = f"%(here)s/{here_to_run}"
        sock = self.layout.socket
        sock = f"{run}/{sock.name}" if sock.parent == self.layout.run else _escape(str(sock))
        lines = [
            f"; {self.layout.instance} ({self.mission.mode}), generated by lamss mission",
            "",
            "[unix_http_server]",
            f"file={sock}",
            "chmod=0700",
            "",
            "[supervisord]",
            f"logfile={run}/log/supervisord.log",
            f"pidfile={run}/supervisord.pid",
            f"childlogdir={run}/log",
            f"identifier=lamss_{self.layout.instance}",
            "",
            "[rpcinterface:supervisor]",
            "supervisor.rpcinterface_factory = supervisor.rpcinterface:make_main_rpcinterface",
            "",
            "[supervisorctl]",
            f"serverurl=unix://{sock}",
        ]
        http_port = self.mission_dict["cruise"].get("supervisor", {}).get("http_port")
        if http_port:
            lines += ["", "[inet_http_server]", f"port=*:{int(http_port)}"]
        else:
            # a glob, so the conf also works before or without the launch writing the file
            includes = [*includes, f"{here_to_run}/{HTTP_CONF[:-5]}*.conf"]
        lines += ["", "[include]", f"files={' '.join(includes)}"]
        return "\n".join(lines) + "\n"


def _output_name(template: str) -> str:
    name = Path(template).name
    return name[:-3] if name.endswith(".j2") else name


def _escape(text: str) -> str:
    """Supervisor expands %(name)s in values, so a literal % is %%."""
    if "\n" in text:
        raise MissionError(f"a supervisord value cannot span lines: {text!r}")
    return text.replace("%", "%%")


def _supervisor_env(env: dict[str, str]) -> str:
    items = []
    for key, value in env.items():
        if any(c in value for c in "\"'\n,"):
            continue  # supervisord mangles these; env.sh still has them
        escaped = value if key in ("PATH",) else _escape(value)
        items.append(f'{key}="{escaped}"')
    return ",".join(items)


def env_sh(env: dict[str, str]) -> str:
    lines = ["# shellcheck shell=sh",
             "# Generated by lamss mission: `. env.sh` to run this subsystem's processes by hand.",
             f'export PATH={shlex.quote(env["LAMSS_PREFIX"] + "/bin")}"${{PATH:+:$PATH}}"',
             f'export LD_LIBRARY_PATH={shlex.quote(env["LAMSS_PREFIX"] + "/lib")}'
             '"${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"']
    lines += [f"export {key}={shlex.quote(value)}" for key, value in env.items()]
    return "\n".join(lines) + "\n"


def _yaml(data: Any) -> str:
    return yaml.safe_dump(data, sort_keys=False, default_flow_style=False, allow_unicode=True, width=100)


def _write(path: Path, text: str) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    return path


def generate(mission: Mission, layout: Layout, platforms: Sequence[str] = (),
             subsystems: Sequence[str] = ()) -> list[Path]:
    if mission.mode == "run" and not platforms:
        raise MissionError("run needs --platform: a machine runs one platform's subsystems")
    return Renderer(mission, layout, select(mission, platforms, subsystems)).write()
