# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""The layered parameter store on its own."""

import pytest

from lamss.mission import MissionError
from lamss.mission.params import Params


def test_later_kinds_win_regardless_of_insertion_order():
    p = Params("x")
    p.add("set", "cli", {"a": 3})
    p.add("default", "defaults", {"a": 1, "b": 1})
    p.add("cruise", "cruise", {"a": 2})
    assert p.merged() == {"a": 3, "b": 1}


def test_nested_dicts_merge_key_by_key():
    p = Params("x")
    p.add("default", "d", {"datum": {"lat": 1.0, "lon": 2.0}})
    p.add("cruise", "c", {"datum": {"lat": 5.0}})
    assert p.merged() == {"datum": {"lat": 5.0, "lon": 2.0}}


def test_lists_replace():
    p = Params("x")
    p.add("default", "d", {"slots": [1, 2, 3]})
    p.add("cruise", "c", {"slots": [4]})
    assert p.merged() == {"slots": [4]}


def test_dotted_override_keys():
    p = Params("x")
    p.add("default", "d", {"datum": {"lat": 1.0, "lon": 2.0}})
    p.add("set", "cli", {"datum.lon": 7.0}, dotted=True, strict=True)
    assert p.merged() == {"datum": {"lat": 1.0, "lon": 7.0}}


def test_strict_override_of_unknown_key_is_an_error():
    p = Params("platform.auv")
    p.add("default", "d", {"max_depth": 100})
    p.add("set", "--set max_dept", {"max_dept": 1}, strict=True)
    with pytest.raises(MissionError, match="sets 'max_dept' on platform.auv.*known: max_depth"):
        p.merged()


def test_dots_in_params_keys_are_rejected():
    with pytest.raises(MissionError, match="must not contain"):
        Params("x").add("default", "d", {"a.b": 1})


def test_values_must_be_plain_data():
    with pytest.raises(MissionError, match="not a string"):
        Params("x").add("default", "d", {"a": object()})


def test_explain_lists_each_layer_and_the_winner():
    p = Params("x")
    p.add("default", "defaults", {"a": 1})
    p.add("platform", "Auv.params", {"a": 2})
    p.add("fleet", "Fleet.platforms", {"b": 0})
    explained = p.explain("a")
    assert [(e.source, e.value, e.wins) for e in explained] == [
        ("defaults", 1, False), ("Auv.params", 2, True)]


def test_explain_a_dict_lists_its_leaves():
    p = Params("x")
    p.add("default", "d", {"datum": {"lat": 1.0, "lon": 2.0}})
    p.add("set", "cli", {"datum.lat": 3.0}, dotted=True)
    assert [(e.key, e.wins) for e in p.explain("datum")] == [
        ("datum.lat", False), ("datum.lon", True), ("datum.lat", True)]
