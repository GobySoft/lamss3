# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""@variant and the merge of combined variants, independent of any missions/ tree."""

import pytest

from lamss.mission import Component, MissionError, Platform, VariantConflict, remove, replace, variant
from lamss.mission.variant import merge_lists, variant_target


class A(Component): pass
class B(Component): pass
class C(Component): pass
class D(Component): pass
class E(Component): pass


class Base(Platform):
    params = dict(depth=10, speed=1)
    components = [A, B, C]


@variant("one")
class One(Base):
    components = replace(Base.components, B, D)


@variant("two")
class Two(Base):
    components = Base.components + [E]
    params = dict(speed=2)


def test_variant_targets_its_base():
    assert variant_target(One) is Base


def test_combination_merges_list_changes_in_place():
    @variant("one_two")
    class OneTwo(One, Two):
        pass
    assert OneTwo.components == [A, D, C, E]
    assert variant_target(OneTwo) is Base


def test_two_parents_replacing_the_same_entry_differently_conflict():
    @variant("three")
    class Three(Base):
        components = replace(Base.components, B, E)

    with pytest.raises(VariantConflict, match="B is replaced differently by One and Three"):
        @variant("one_three")
        class OneThree(One, Three):
            pass


def test_explicit_attribute_resolves_a_conflict():
    @variant("three")
    class Three(Base):
        components = replace(Base.components, B, E)

    @variant("one_three")
    class OneThree(One, Three):
        components = [A, D, E, C]
    assert OneThree.components == [A, D, E, C]


def test_conflicting_params_are_reported():
    @variant("slow")
    class Slow(Base):
        params = dict(speed=0.5)

    with pytest.raises(VariantConflict, match="params 'speed': Two sets 2, Slow sets 0.5"):
        @variant("two_slow")
        class TwoSlow(Two, Slow):
            pass


def test_conflicting_spec_overrides_are_reported():
    @variant("x")
    class X(Base):
        components = replace(Base.components, A, A(level=1))

    @variant("y")
    class Y(Base):
        components = replace(Base.components, A, A(level=2))

    with pytest.raises(VariantConflict, match="A is changed by both X and Y"):
        @variant("xy")
        class XY(X, Y):
            pass


def test_same_removal_in_both_parents_is_fine():
    @variant("x")
    class X(Base):
        components = remove(Base.components, C)

    @variant("y")
    class Y(Base):
        components = [E] + remove(Base.components, C)

    @variant("xy")
    class XY(X, Y):
        pass
    assert XY.components == [E, A, B]


def test_variants_of_different_concepts_cannot_combine():
    class Other(Platform):
        pass

    @variant("o")
    class OtherVariant(Other):
        pass

    with pytest.raises(MissionError, match="combines variants of different concepts"):
        @variant("mixed")
        class Mixed(One, OtherVariant):
            pass


def test_variant_must_decorate_a_concept():
    with pytest.raises(MissionError, match="must decorate a concept class"):
        @variant("bare")
        class Bare:
            pass


def test_merge_keeps_additions_in_order():
    assert merge_lists([A, B], [("p", [A, C, D, B]), ("q", [A, B, E])]) == [A, C, D, B, E]
