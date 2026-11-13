# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Where in the mission files a value or a concept comes from, as file and line.

A `Ref` names a class attribute (`Trial.params`, `Pair.platforms`) and, within it, a list entry
and a key. `locate` parses the class's file and returns the line of that key, or of the closest
thing it can find statically: the entry, then the attribute's assignment, then the class. Values
built in code (a comprehension, `{**Base.params, ...}`) therefore point at their assignment.
"""

from __future__ import annotations

import ast
import functools
import inspect
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Optional

Location = tuple[str, int]

_SUBSYSTEM_ATTR = re.compile(r"^subsystems\[(.+)\]$")


@dataclass(frozen=True)
class Ref:
    """`cls.attr`, narrowed to the list `entry` (by identity) and to a fixed `key` when given."""

    cls: type
    attr: str
    entry: Any = None
    key: Optional[str] = None


def locate(ref: Optional[Ref], key: Optional[str] = None) -> Optional[Location]:
    """The file and line of `ref`, and of `key` (dotted, within the entry or the attribute) when
    `ref` has no fixed key of its own."""
    if ref is None:
        return None
    found = _class(ref.cls)
    if found is None:
        return None
    file, class_node = found
    sub = _SUBSYSTEM_ATTR.match(ref.attr)
    attr = "subsystems" if sub else ref.attr
    assignment = _assignment(class_node, attr)
    if assignment is None:
        return file, class_node.lineno
    node: ast.AST = assignment.value
    line = assignment.lineno
    if ref.entry is not None:
        entries = ref.cls.__dict__.get(attr)
        if sub:
            spec = next((s for s in entries or [] if getattr(s, "_spec_name", None) == sub.group(1)),
                        None)
            spec_node = _element(node, entries, spec)
            components = _keyword(spec_node, "components", 1) if spec_node is not None else None
            element = _element(components, getattr(spec, "_spec_components", None), ref.entry)
        else:
            element = _element(node, entries, ref.entry)
        if element is None:
            return file, line
        node, line = element, element.lineno
    wanted = ref.key if ref.key is not None else key
    if wanted:
        line = _key_line(node, wanted.split("."), whole=wanted) or line
    return file, line


def class_location(cls: type) -> Optional[Location]:
    found = _class(cls)
    return (found[0], found[1].lineno) if found else None


def method_location(cls: type, name: str) -> Optional[Location]:
    owner = next((c for c in cls.__mro__ if name in c.__dict__), None)
    if owner is None:
        return None
    try:
        return inspect.getsourcefile(owner) or "", inspect.getsourcelines(owner.__dict__[name])[1]
    except (OSError, TypeError):
        return None


@functools.lru_cache(maxsize=None)
def _tree(file: str, mtime: float) -> ast.Module:
    return ast.parse(Path(file).read_text(), file)


def _class(cls: type) -> Optional[tuple[str, ast.ClassDef]]:
    try:
        file = inspect.getsourcefile(cls)
        line = inspect.getsourcelines(cls)[1]
    except (OSError, TypeError):
        return None
    if not file:
        return None
    candidates = [n for n in ast.walk(_tree(file, Path(file).stat().st_mtime))
                  if isinstance(n, ast.ClassDef) and n.name == cls.__name__]
    for node in candidates:
        if line in [node.lineno] + [d.lineno for d in node.decorator_list]:
            return file, node
    return (file, candidates[0]) if candidates else None


def _assignment(class_node: ast.ClassDef, attr: str) -> Optional[ast.Assign | ast.AnnAssign]:
    for stmt in class_node.body:
        if isinstance(stmt, ast.Assign) and any(isinstance(t, ast.Name) and t.id == attr
                                                for t in stmt.targets):
            return stmt
        if isinstance(stmt, ast.AnnAssign) and isinstance(stmt.target, ast.Name) \
                and stmt.target.id == attr and stmt.value is not None:
            return stmt
    return None


def _element(node: Optional[ast.AST], entries: Any, entry: Any) -> Optional[ast.expr]:
    """The literal list element written for the runtime `entry`, matched by position."""
    if not isinstance(node, (ast.List, ast.Tuple)) or not isinstance(entries, (list, tuple)):
        return None
    index = next((i for i, e in enumerate(entries) if e is entry), None)
    if index is None or len(node.elts) != len(entries):
        return None
    return node.elts[index]


def _keyword(call: ast.AST, name: str, position: int) -> Optional[ast.expr]:
    if not isinstance(call, ast.Call):
        return None
    for kw in call.keywords:
        if kw.arg == name:
            return kw.value
    return call.args[position] if len(call.args) > position else None


def _key_line(node: ast.AST, segments: list[str], whole: str) -> Optional[int]:
    """The line of the deepest of `segments` found in a dict literal, a dict(...) or a call's
    keywords; a dotted key written whole ("platform.glider.speed") matches too."""
    if not segments:
        return None
    if isinstance(node, ast.Dict):
        for k, v in zip(node.keys, node.values):
            if isinstance(k, ast.Constant) and k.value == whole:
                return k.lineno
            if isinstance(k, ast.Constant) and k.value == segments[0]:
                return _key_line(v, segments[1:], ".".join(segments[1:])) or k.lineno
    if isinstance(node, ast.Call):
        for kw in node.keywords:
            if kw.arg == segments[0]:
                return _key_line(kw.value, segments[1:], ".".join(segments[1:])) or kw.value.lineno
    return None
