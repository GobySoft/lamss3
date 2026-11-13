# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""`@variant`: named alternatives of a concept, applied only when requested.

A variant subclasses the concept it changes. A combination of variants is a variant that inherits
from them; its list attributes (components, subsystems, ...) are merged from each parent's changes
to the common base, and any two parents changing the same thing differently is an error raised
when the class is defined, unless the combination sets that attribute itself.
"""

from __future__ import annotations

from typing import Any, Callable, Sequence, TypeVar

from .concept import Concept, entry_key
from .errors import MissionError, VariantConflict
from .params import flatten

T = TypeVar("T", bound=type)

LIST_ATTRIBUTES = ("components", "sim_components", "run_components", "subsystems", "platforms",
                   "processes", "templates", "requires")
DICT_ATTRIBUTES = ("params", "overrides")
_VARIANT = "_lamss_variant"
_IGNORED = {"__module__", "__qualname__", "__doc__", "__dict__", "__weakref__",
            "__annotations__", "__orig_bases__", "__firstlineno__", "__static_attributes__",
            _VARIANT}


def is_variant(cls: type) -> bool:
    return _VARIANT in cls.__dict__


def variant_name(cls: type) -> str:
    return cls.__dict__[_VARIANT]


def variant_target(cls: type) -> type:
    """The concept a variant class changes: its nearest non-variant ancestor."""
    if not is_variant(cls):
        return cls
    targets = {variant_target(base) for base in cls.__bases__}
    if len(targets) != 1:
        names = ", ".join(sorted(t.__name__ for t in targets))
        raise MissionError(f"variant {cls.__name__} combines variants of different concepts: {names}")
    return targets.pop()


def variant_classes(cls: type) -> list[type]:
    """The variant classes in `cls`'s MRO, lowest precedence first."""
    return [c for c in reversed(cls.__mro__) if is_variant(c)]


def variant(name: str) -> Callable[[T], T]:
    if not name or not name.replace("_", "").isalnum():
        raise MissionError(f"variant name {name!r} must be letters, digits and '_'")

    def decorate(cls: T) -> T:
        if not (isinstance(cls, type) and issubclass(cls, Concept)):
            raise MissionError(f"@variant({name!r}) must decorate a concept class")
        setattr(cls, _VARIANT, name)
        target = variant_target(cls)
        parents = [b for b in cls.__bases__ if is_variant(b)]
        if len(parents) > 1:
            _combine(cls, target, parents)
        return cls

    return decorate


def _own_variant_values(cls: type) -> dict[str, tuple[str, Any]]:
    """Attributes set by the variant classes of `cls`: name -> (setting class, value)."""
    out: dict[str, tuple[str, Any]] = {}
    for c in variant_classes(cls):
        for attr, value in c.__dict__.items():
            if attr not in _IGNORED:
                out[attr] = (c.__name__, value)
    return out


def _combine(cls: type, target: type, parents: Sequence[type]) -> None:
    own = cls.__dict__
    conflicts: list[str] = []

    for attr in DICT_ATTRIBUTES:
        if attr in own:
            continue
        seen: dict[str, tuple[str, Any]] = {}
        for parent in parents:
            for c in variant_classes(parent):
                for key, value in flatten(c.__dict__.get(attr, {}), dotted=attr == "overrides"):
                    if key in seen and seen[key][1] != value and seen[key][0] != c.__name__:
                        conflicts.append(f"{attr} '{key}': {seen[key][0]} sets {seen[key][1]!r}, "
                                         f"{c.__name__} sets {value!r}")
                    seen[key] = (c.__name__, value)

    for attr in LIST_ATTRIBUTES:
        if attr in own or not hasattr(target, attr):
            continue
        base = list(getattr(target, attr))
        changed = [(p.__name__, list(getattr(p, attr))) for p in parents
                   if list(getattr(p, attr)) != base]
        if len(changed) > 1:
            try:
                setattr(cls, attr, merge_lists(base, changed))
            except VariantConflict as e:
                conflicts.append(f"{attr}: {e}")
        elif changed:
            setattr(cls, attr, changed[0][1])

    values = [_own_variant_values(p) for p in parents]
    for attr in sorted(set().union(*values)):
        if attr in own or attr in LIST_ATTRIBUTES or attr in DICT_ATTRIBUTES:
            continue
        setters = [v[attr] for v in values if attr in v]
        if any(s[1] is not setters[0][1] and s[1] != setters[0][1] for s in setters[1:]):
            conflicts.append(f"'{attr}' is set by " + " and ".join(s[0] for s in setters))

    if conflicts:
        raise VariantConflict(
            f"variant {cls.__name__} ({variant_name(cls)}) combines "
            f"{', '.join(p.__name__ for p in parents)}, which conflict; set these in "
            f"{cls.__name__} to resolve them:\n  " + "\n  ".join(conflicts))


def _changes(base: list[Any], entries: list[Any]) -> tuple[dict[Any, Any], list[tuple[Any, Any]]]:
    """How `entries` differs from `base`: edits of base entries by key ("removed" or the new
    entry), and added entries, each with the key of the base entry it follows (None: first)."""
    base_keys = [entry_key(e) for e in base]
    by_key = {entry_key(e): e for e in entries}
    edits = {}
    for key, entry in zip(base_keys, base):
        if key not in by_key:
            edits[key] = "removed"
        elif by_key[key] != entry:
            edits[key] = by_key[key]
    added = []
    anchor = None
    for entry in entries:
        key = entry_key(entry)
        if key in base_keys:
            anchor = key
        else:
            added.append((anchor, entry))
    return edits, added


def merge_lists(base: list[Any], changed: Sequence[tuple[str, list[Any]]]) -> list[Any]:
    """Applies every parent's changes to `base`; raises VariantConflict when two disagree,
    including two parents replacing the same entry with different ones."""
    base_keys = [entry_key(e) for e in base]
    edits: dict[Any, tuple[str, Any]] = {}
    additions: dict[Any, tuple[str, Any, Any]] = {}
    problems = []
    for parent, entries in changed:
        parent_edits, added = _changes(base, entries)
        for key, edit in parent_edits.items():
            if key in edits:
                other, other_edit = edits[key]
                if edit == "removed" and other_edit == "removed":
                    i = base_keys.index(key)
                    before = base_keys[i - 1] if i else None
                    mine = [entry_key(e) for a, e in added if a == before]
                    theirs = [k for k, (p, a, _) in additions.items() if p == other and a == before]
                    if mine != theirs:
                        problems.append(f"{_describe(key)} is replaced differently by {other} "
                                        f"and {parent}")
                elif edit != other_edit:
                    problems.append(f"{_describe(key)} is changed by both {other} and {parent}")
            else:
                edits[key] = (parent, edit)
        for anchor, entry in added:
            key = entry_key(entry)
            if key in additions:
                if additions[key][2] != entry:
                    problems.append(f"{_describe(key)} is added differently by "
                                    f"{additions[key][0]} and {parent}")
                continue
            additions[key] = (parent, anchor, entry)
    if problems:
        raise VariantConflict("; ".join(problems))

    out: list[tuple[Any, Any]] = []
    for key, entry in zip(base_keys, base):
        edit = edits.get(key, (None, None))[1]
        if edit != "removed":
            out.append((key, entry if edit is None else edit))
    inserted: dict[Any, int] = {}
    for key, (_, anchor, entry) in additions.items():
        present = anchor
        while present is not None and present not in [k for k, _ in out]:
            i = base_keys.index(present)
            present = base_keys[i - 1] if i else None
        index = [k for k, _ in out].index(present) + 1 if present is not None else 0
        index += inserted.get(present, 0)
        inserted[present] = inserted.get(present, 0) + 1
        out.insert(index, (key, entry))
    return [entry for _, entry in out]


def _describe(key: Any) -> str:
    kind, name = key
    if isinstance(kind, type):
        return kind.__name__ + (f"({name!r})" if name else "")
    return f"{kind} {name!r}"
