# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Class discovery over the active toolboxes' missions/ trees."""

import pytest

from lamss.mission import Component, MissionError, Platform


def test_classes_are_registered_by_path_and_short_name(tree):
    registry = tree()
    glider = registry.get("glider")
    assert registry.get("platform/auv/glider") is glider
    assert registry.path_of(glider) == "platform/auv/glider"
    assert issubclass(glider, registry.get("auv"))
    assert registry.toolbox_of(glider).name == "extra"
    assert registry.toolbox_of(registry.get("auv")).name == "core"


def test_built_in_bases_are_the_tree_roots(tree):
    registry = tree()
    assert registry.get("platform") is Platform
    assert registry.get("component") is Component
    assert set(registry.cruises()) == {"trial"}


def test_toolboxes_extend_the_same_directory(tree):
    # extra/missions/platform/auv/ has no auv.py: the class comes from core
    registry = tree()
    import missions.platform.auv as auv
    assert len(auv.__path__) == 2
    assert auv.Auv is registry.get("auv")


def test_variants_are_found_next_to_their_base(tree):
    registry = tree()
    assert {c.__name__ for c in registry.variants()["deep"]} == {"GliderDeep"}
    assert set(registry.variants()) == {"deep", "quiet", "acomms", "deep_quiet", "fast"}


def test_inactive_toolbox_is_not_loaded(tree):
    registry = tree(remove=[k for k in __import__("conftest").BASIC if k.startswith("extra/")],
                    toolboxes=("core",))
    assert registry.find("glider") is None
    assert registry.find("auv") is not None


def test_name_clash_is_an_error(tree):
    with pytest.raises(MissionError, match="name clash.*'udp'"):
        tree({"extra/missions/component/udp/udp.py": "class Udp2(Component): pass\n"})


def test_same_file_in_two_toolboxes_is_an_error(tree):
    with pytest.raises(MissionError, match="defined twice"):
        tree({"extra/missions/platform/auv/auv.py": "class Auv(Platform): pass\n"})


def test_subdirectory_must_subclass_its_parent(tree):
    with pytest.raises(MissionError, match="must subclass Auv"):
        tree({"extra/missions/platform/auv/boat/boat.py": "class Boat(Platform): pass\n"})


def test_class_kind_must_match_its_tree(tree):
    with pytest.raises(MissionError, match="under platform/ but is not a Platform"):
        tree({"extra/missions/platform/thing/thing.py": "class Thing(Component): pass\n"})


def test_unknown_top_level_directory_is_an_error(tree):
    with pytest.raises(MissionError, match="not a concept tree"):
        tree({"extra/missions/vehicles/x/x.py": "class X(Platform): pass\n"})


def test_reserved_directories_are_skipped(tree):
    registry = tree({"extra/missions/build/trial/junk.py": "raise RuntimeError('imported')\n",
                     "extra/missions/templates/x.j2": "{{ x }}\n"})
    assert registry.find("trial") is not None


def test_stray_class_in_a_variant_file_is_an_error(tree):
    with pytest.raises(MissionError, match="neither platform/auv/glider's class"):
        tree({"extra/missions/platform/auv/glider/glider_extra.py": "class Extra(Platform): pass\n"})


def test_import_errors_name_the_file(tree):
    with pytest.raises(MissionError, match=r"broken\.py:\d+.*NameError"):
        tree({"extra/missions/component/broken/broken.py": "class Broken(Component):\n    x = nope\n"})


def test_duplicate_variant_for_one_concept_is_an_error(tree):
    with pytest.raises(MissionError, match="variant 'deep' is defined twice"):
        tree({"extra/missions/platform/auv/glider/glider_deeper.py": """
            from missions.platform.auv.glider import Glider
            @variant("deep")
            class GliderDeeper(Glider):
                params = dict(max_depth=2000)
            """})
