# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Layered parameters with provenance.

Every concept's parameters are a stack of layers, each an explicit dict from one source. Later
kinds override earlier ones, and within a kind later layers override earlier ones. Nested dicts are
merged key by key; any other value (lists included) replaces what was there.
"""

from __future__ import annotations

import copy
from dataclasses import dataclass
from typing import Any, Iterator, Mapping, Optional

from .errors import MissionError
from .location import Location, Ref, locate

LAYER_KINDS = ("default", "platform", "fleet", "cruise", "variant", "set")


@dataclass(frozen=True)
class Layer:
    kind: str
    source: str
    values: Mapping[str, Any]
    # an override may only change keys that an earlier layer defined
    strict: bool = False
    ref: Optional[Ref] = None


@dataclass(frozen=True)
class Explanation:
    key: str
    kind: str
    source: str
    value: Any
    wins: bool
    location: Optional[Location] = None


def flatten(values: Mapping[str, Any], prefix: str = "", dotted: bool = False) -> Iterator[tuple[str, Any]]:
    for key, value in values.items():
        if not isinstance(key, str) or not key:
            raise MissionError(f"parameter key {key!r} must be a non-empty string")
        if "." in key and not dotted:
            raise MissionError(f"parameter key {prefix + key!r} must not contain '.'")
        if isinstance(value, Mapping) and value:
            yield from flatten(value, f"{prefix}{key}.")
        else:
            yield prefix + key, plain(value)


def plain(value: Any) -> Any:
    """A copy made of dicts, lists and scalars only, so templates and YAML see plain data."""
    if isinstance(value, Mapping):
        return {k: plain(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [plain(v) for v in value]
    if value is None or isinstance(value, (str, int, float, bool)):
        return value
    raise MissionError(f"parameter value {value!r} is not a string, number, bool, list or dict")


def unflatten(flat: Mapping[str, Any]) -> dict[str, Any]:
    out: dict[str, Any] = {}
    for key, value in flat.items():
        node = out
        *parents, leaf = key.split(".")
        for part in parents:
            node = node.setdefault(part, {})
        node[leaf] = copy.deepcopy(value)
    return out


def _assign(merged: dict[str, Any], key: str, value: Any) -> None:
    if key in merged and not (isinstance(value, dict) and value):
        merged[key] = value
        return
    for existing in [k for k in merged if k.startswith(key + ".") or key.startswith(k + ".")]:
        del merged[existing]
    if isinstance(value, dict) and value:
        merged.update(flatten(value, key + "."))
    else:
        merged[key] = value


def _known(merged: Mapping[str, Any], key: str) -> bool:
    return key in merged or any(k.startswith(key + ".") for k in merged)


class Params:
    """The parameter layers of one concept; `owner` names it in error messages."""

    def __init__(self, owner: str):
        self.owner = owner
        self._layers: list[Layer] = []

    def add(self, kind: str, source: str, values: Mapping[str, Any], *, dotted: bool = False,
            strict: bool = False, ref: Optional[Ref] = None) -> None:
        if kind not in LAYER_KINDS:
            raise ValueError(f"unknown layer kind {kind!r}")
        if values:
            self._layers.append(Layer(kind, source, dict(flatten(values, dotted=dotted)), strict,
                                      ref))

    def layers(self) -> list[Layer]:
        return sorted(self._layers, key=lambda layer: LAYER_KINDS.index(layer.kind))

    def flat(self) -> dict[str, Any]:
        merged: dict[str, Any] = {}
        for layer in self.layers():
            for key, value in layer.values.items():
                if layer.strict and not _known(merged, key):
                    known = ", ".join(sorted(merged)) or "none"
                    raise MissionError(
                        f"{layer.source} sets '{key}' on {self.owner}, which has no such "
                        f"parameter (known: {known})")
                _assign(merged, key, copy.deepcopy(value))
        return merged

    def merged(self) -> dict[str, Any]:
        return unflatten(self.flat())

    def explain(self, key: str) -> list[Explanation]:
        """Every layer that set `key` or a key beneath it, lowest precedence first."""
        final = self.flat()
        out = []
        for layer in self.layers():
            for k, value in layer.values.items():
                if k == key or k.startswith(key + ".") or key.startswith(k + "."):
                    out.append(Explanation(k, layer.kind, layer.source, value, False,
                                           locate(layer.ref, k)))
        winners = set()
        for i in reversed(range(len(out))):
            e = out[i]
            if e.key in final and e.key not in winners and final[e.key] == e.value:
                winners.add(e.key)
                out[i] = Explanation(e.key, e.kind, e.source, e.value, True, e.location)
        return out
