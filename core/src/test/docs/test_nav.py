# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Expanding docs/nav.yml against the staged pages."""

from lamss.docs.nav import Nav

STAGED = {
    "index.md", "tool/index.md", "tool/build.md",
    "core/index.md", "core/apps/pViewMessage.md", "core/apps/iSerialNMEA.md",
    "core/libraries/henrik_util.md", "core/guide/intro.md",
    "core/plan/core.md", "core/plan/claude/README.md", "core/plan/claude/00-overview.md",
    "sonar/index.md",
    "reference/messages/index.md", "reference/messages/core.md",
}


def test_missing_pages_and_empty_sections_are_dropped():
    nav = Nav(STAGED, ["core", "sonar"])
    assert nav.expand([{"Home": "index.md"}, {"Gone": "gone.md"},
                       {"Tool": ["tool/index.md", "tool/missing.md"]}, {"Empty": ["x.md"]}]) == \
        [{"Home": "index.md"}, {"Tool": ["tool/index.md"]}]


def test_toolboxes_list_index_then_sections_in_order_without_plans():
    nav = Nav(STAGED, ["core", "sonar"])
    assert nav.expand([{"Toolboxes": "@toolboxes"}]) == [{"Toolboxes": [
        {"core": ["core/index.md",
                  {"Guide": ["core/guide/intro.md"]},
                  {"Apps": ["core/apps/iSerialNMEA.md", "core/apps/pViewMessage.md"]},
                  {"Libraries": ["core/libraries/henrik_util.md"]}]},
        {"sonar": ["sonar/index.md"]},
    ]}]


def test_messages_and_directories():
    nav = Nav(STAGED, ["core", "sonar"])
    assert nav.expand([{"Messages": "@messages"}]) == [
        {"Messages": ["reference/messages/index.md", {"core": "reference/messages/core.md"}]}]
    assert nav.expand([{"Plans": "@dir:core/plan/claude"}]) == [
        {"Plans": ["core/plan/claude/README.md", "core/plan/claude/00-overview.md"]}]
