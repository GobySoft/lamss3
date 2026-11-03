# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Discovers the concept classes in the active toolboxes' `missions/` trees.

A directory `x/` holds `x.py` defining class `X`; its subdirectories hold subclasses, and its other
`.py` files hold variants. Every tree is importable as the `missions` package, merged across
toolboxes, so `mit/missions/platform/auv/propellor/bluefin21/macrura/macrura.py` can say
``from missions.platform.auv.propellor.bluefin21 import Bluefin21`` whichever toolbox defines it.
"""

from __future__ import annotations

import importlib
import importlib.abc
import importlib.machinery
import importlib.util
import sys
import traceback
from dataclasses import dataclass
from pathlib import Path
from types import ModuleType
from typing import Iterator, Optional, Sequence

from .concept import BASE_CLASSES, Component, Concept, Cruise
from .errors import MissionError
from .variant import is_variant, variant_name, variant_target

PACKAGE = "missions"
# directories of a missions/ tree that hold no classes (mission output lives beside the classes)
RESERVED = {"templates", "data", "build", "log", "run", "test"}


@dataclass(frozen=True)
class Toolbox:
    name: str
    missions: Path


class _EmptyLoader(importlib.abc.Loader):
    def create_module(self, spec):  # noqa: D102
        return None

    def exec_module(self, module: ModuleType) -> None:
        pass


class _Finder(importlib.abc.MetaPathFinder):
    def __init__(self, roots: Sequence[Path]):
        self.roots = list(roots)

    def find_spec(self, fullname, path=None, target=None):
        if fullname != PACKAGE and not fullname.startswith(PACKAGE + "."):
            return None
        parts = fullname.split(".")[1:]
        dirs = [r.joinpath(*parts) for r in self.roots if r.joinpath(*parts).is_dir()]
        files: list[Path] = []
        if parts:
            files = [f for r in self.roots
                     if (f := r.joinpath(*parts[:-1], parts[-1] + ".py")).is_file()]
            # x/x.py is the code of package x, never a module of its own
            if len(parts) > 1 and parts[-1] == parts[-2]:
                files = []
        if dirs and files:
            raise ImportError(f"{fullname} is both a directory and a file: {dirs[0]}, {files[0]}")
        if dirs:
            code = [d / (parts[-1] + ".py") for d in dirs if parts and (d / (parts[-1] + ".py")).is_file()]
            if len(code) > 1:
                raise ImportError(f"{fullname} is defined twice: {code[0]} and {code[1]}")
            locations = [str(d) for d in dirs]
            if code:
                return importlib.util.spec_from_file_location(
                    fullname, code[0], submodule_search_locations=locations)
            spec = importlib.machinery.ModuleSpec(fullname, _EmptyLoader(), is_package=True)
            spec.submodule_search_locations = locations
            return spec
        if len(files) > 1:
            raise ImportError(f"{fullname} is defined twice: {files[0]} and {files[1]}")
        if files:
            return importlib.util.spec_from_file_location(fullname, files[0])
        return None


class Registry:
    """The concept classes of a set of toolboxes, by path (`platform/auv`) and short name (`auv`).

    Only one registry is active per process: loading one replaces the `missions` package."""

    def __init__(self, toolboxes: Sequence[Toolbox]):
        self.toolboxes = [t for t in toolboxes if t.missions.is_dir()]
        self._by_path: dict[str, type[Concept]] = dict(BASE_CLASSES)
        self._path_of: dict[type, str] = {cls: path for path, cls in BASE_CLASSES.items()}
        self._by_name: dict[str, type[Concept]] = {}
        self._toolbox_of: dict[type, Toolbox] = {}
        self._variants: dict[str, list[type[Concept]]] = {}
        self._loaded = False

    def load(self) -> "Registry":
        _activate(self.toolboxes)
        for parts in self._directories(()):
            self._load_directory(parts)
        self._check_variants()
        self._loaded = True
        return self

    def _directories(self, parts: tuple[str, ...]) -> Iterator[tuple[str, ...]]:
        names = set()
        for tb in self.toolboxes:
            d = tb.missions.joinpath(*parts)
            if d.is_dir():
                names.update(p.name for p in d.iterdir() if p.is_dir())
        for name in sorted(names):
            if name.startswith((".", "_")) or name in RESERVED:
                continue
            if not parts and name not in BASE_CLASSES:
                raise MissionError(
                    f"missions/{name}/ is not a concept tree; the top-level directories are "
                    f"{', '.join(BASE_CLASSES)} and {', '.join(sorted(RESERVED))}")
            yield parts + (name,)
            yield from self._directories(parts + (name,))

    def _import(self, module: str) -> ModuleType:
        try:
            return importlib.import_module(module)
        except MissionError:
            raise
        except Exception as e:
            tb = traceback.extract_tb(e.__traceback__)
            where = next((f"{f.filename}:{f.lineno}" for f in reversed(tb)
                          if self._in_tree(f.filename)), module)
            raise MissionError(f"cannot load {module} ({where}): {type(e).__name__}: {e}") from e

    def _in_tree(self, filename: str) -> bool:
        return any(Path(filename).is_relative_to(t.missions) for t in self.toolboxes)

    def _load_directory(self, parts: tuple[str, ...]) -> None:
        path = "/".join(parts)
        module = self._import(".".join((PACKAGE,) + parts))
        files = [Path(f) for f in (getattr(module, "__file__", None),) if f]
        if len(parts) > 1 or files:
            if len(parts) == 1:
                raise MissionError(f"{files[0]}: {path} is built in (lamss.mission.{BASE_CLASSES[path].__name__})")
            parent = self._by_path.get("/".join(parts[:-1]))
            main = self._main_class(module, parts, files)
            if main is not None:
                root = Component if parts[0] == "environment" and "component" in parts else BASE_CLASSES[parts[0]]
                if not issubclass(main, root):
                    raise MissionError(f"{main.__name__} ({files[0]}) is under {parts[0]}/ but is not a "
                                       f"{root.__name__}")
                if parent is not None and not issubclass(main, parent):
                    raise MissionError(f"{main.__name__} ({files[0]}) must subclass {parent.__name__}, "
                                       f"the class of the directory above it")
                self._register(main, path, self._toolbox(files[0]))
            for cls in self._concepts(module):
                if is_variant(cls):
                    self._add_variant(cls, files[0])
        for tb in self.toolboxes:
            d = tb.missions.joinpath(*parts)
            if not d.is_dir():
                continue
            for f in sorted(d.glob("*.py")):
                if f.stem == parts[-1] or f.stem.startswith("_"):
                    continue
                sub = self._import(".".join((PACKAGE,) + parts + (f.stem,)))
                for cls in self._concepts(sub):
                    if not is_variant(cls):
                        raise MissionError(
                            f"{cls.__name__} ({f}) is neither {path}'s class (which belongs in "
                            f"{parts[-1]}.py) nor a @variant")
                    self._add_variant(cls, f)

    def _concepts(self, module: ModuleType) -> list[type[Concept]]:
        return [v for v in vars(module).values()
                if isinstance(v, type) and issubclass(v, Concept) and v.__module__ == module.__name__]

    def _main_class(self, module: ModuleType, parts: tuple[str, ...], files: list[Path]) -> Optional[type]:
        if not files:
            return None
        candidates = [c for c in self._concepts(module) if not is_variant(c)]
        camel = parts[-1].replace("_", "").lower()
        if len(candidates) > 1:
            named = [c for c in candidates if c.__name__.lower() == camel]
            if len(named) != 1:
                raise MissionError(f"{files[0]} defines {', '.join(c.__name__ for c in candidates)}; "
                                   f"a directory holds one class")
            candidates = named
        if not candidates:
            raise MissionError(f"{files[0]} defines no concept class")
        return candidates[0]

    def _toolbox(self, file: Path) -> Toolbox:
        return next(t for t in self.toolboxes if file.is_relative_to(t.missions))

    def _register(self, cls: type[Concept], path: str, toolbox: Toolbox) -> None:
        if cls in self._path_of:
            raise MissionError(f"{cls.__name__} is registered as both {self._path_of[cls]} and {path}")
        name = path.rsplit("/", 1)[-1]
        if name in self._by_name:
            raise MissionError(f"name clash: {path} ({toolbox.name}) and {self._path_of[self._by_name[name]]} "
                               f"({self._toolbox_of[self._by_name[name]].name}) are both '{name}'")
        self._by_path[path] = cls
        self._path_of[cls] = path
        self._by_name[name] = cls
        self._toolbox_of[cls] = toolbox

    def _add_variant(self, cls: type[Concept], file: Path) -> None:
        self._variants.setdefault(variant_name(cls), []).append(cls)
        self._toolbox_of[cls] = self._toolbox(file)

    def _check_variants(self) -> None:
        for name, classes in self._variants.items():
            targets: dict[type, type] = {}
            for cls in classes:
                target = variant_target(cls)
                if target not in self._path_of:
                    raise MissionError(f"variant {cls.__name__} ({name}) changes {target.__name__}, "
                                       f"which is not a registered concept")
                if target in targets:
                    raise MissionError(f"variant '{name}' is defined twice for {self._path_of[target]}: "
                                       f"{targets[target].__name__} and {cls.__name__}")
                targets[target] = cls

    # lookups

    def find(self, ref: str) -> Optional[type[Concept]]:
        return self._by_path.get(ref) if "/" in ref else self._by_name.get(ref, self._by_path.get(ref))

    def get(self, ref: str, base: type = Concept) -> type[Concept]:
        cls = self.find(ref)
        if cls is None:
            raise MissionError(f"no {base.kind if base is not Concept else 'concept'} named '{ref}' "
                               f"in the active toolboxes ({', '.join(t.name for t in self.toolboxes)})")
        if not issubclass(cls, base):
            raise MissionError(f"'{ref}' is a {cls.kind}, not a {base.kind}")
        return cls

    def path_of(self, cls: type) -> Optional[str]:
        return self._path_of.get(cls)

    def name_of(self, cls: type) -> Optional[str]:
        path = self._path_of.get(cls)
        return path.rsplit("/", 1)[-1] if path and path not in BASE_CLASSES else None

    def toolbox_of(self, cls: type) -> Optional[Toolbox]:
        return self._toolbox_of.get(cls)

    def directory_of(self, cls: type) -> Optional[Path]:
        module = sys.modules.get(cls.__module__)
        file = getattr(module, "__file__", None)
        return Path(file).parent if file and self._in_tree(file) else None

    def classes(self, base: type = Concept) -> dict[str, type[Concept]]:
        """Registered classes deriving from `base` (the built-in bases excluded), by path."""
        return {p: c for p, c in self._by_path.items()
                if issubclass(c, base) and p not in BASE_CLASSES}

    def cruises(self) -> dict[str, type[Cruise]]:
        return {self.name_of(c): c for c in self.classes(Cruise).values()}  # type: ignore[misc]

    def variants(self, name: Optional[str] = None) -> dict[str, list[type[Concept]]]:
        if name is None:
            return dict(self._variants)
        return {name: list(self._variants.get(name, []))}

    def component_name(self, cls: type) -> Optional[str]:
        """A component's name in the context: the directory beneath `component/`, so every acomms
        implementation is `acomms` (`environment/component/<name>/...` likewise)."""
        for c in cls.__mro__:
            if "name" in c.__dict__ and isinstance(c.__dict__["name"], str):
                return c.__dict__["name"]
            path = self._path_of.get(c)
            if path and issubclass(c, Component) and path not in BASE_CLASSES:
                parts = path.split("/")
                if "component" in parts and parts.index("component") + 1 < len(parts):
                    return parts[parts.index("component") + 1]
                return parts[-1]
        return None

    def concept_name(self, cls: type) -> Optional[str]:
        """A cruise's, fleet's, platform's or environment's name: its directory, or `name`."""
        for c in cls.__mro__:
            if "name" in c.__dict__ and isinstance(c.__dict__["name"], str):
                return c.__dict__["name"]
            if is_variant(c):
                continue
            name = self.name_of(c)
            if name:
                return name
        return None


def _activate(toolboxes: Sequence[Toolbox]) -> None:
    for finder in [f for f in sys.meta_path if isinstance(f, _Finder)]:
        sys.meta_path.remove(finder)
    for module in [m for m in sys.modules if m == PACKAGE or m.startswith(PACKAGE + ".")]:
        del sys.modules[module]
    importlib.invalidate_caches()
    sys.meta_path.insert(0, _Finder([t.missions for t in toolboxes]))

