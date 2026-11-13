# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Link rewriting: pages are written with repository-relative links (as on GitHub) and staged
under different paths for the site."""

from lamss.docs.links import LinkMap, Repository, rewrite_markdown

URL = "https://github.com/GobySoft/lamss3"


def links():
    pages = {
        "index.md": "docs/index.md",
        "tool/build.md": "docs/tool/build.md",
        "core/apps/pViewMessage.md": "core/src/doc/apps/pViewMessage.md",
        "core/plan/claude/11-documentation.md": "core/src/doc/plan/claude/11-documentation.md",
        "reference/dccl-ids.md": "core/src/lib/protobuf/lamss/DCCL_IDS.md",
    }
    files = {"AGENTS.md", "core/src/bin/pViewMessage/ViewMessage.cpp",
             "core/src/lib/protobuf/lamss/DCCL_IDS.md", "docs/index.md"}
    repos = [Repository("", URL, "abc123", files),
             Repository("icex/", "https://github.com/GobySoft/lamss3-icex", "def456",
                        {"src/bin/pIcex/Icex.cpp"})]
    return LinkMap(pages, repos, staged_files=set(pages) | {"reference/cli.md"})


def test_link_to_published_page_follows_its_staged_path():
    assert links().resolve("../../../lib/protobuf/lamss/DCCL_IDS.md#core",
                           "core/plan/claude/11-documentation.md") == \
        "../../../reference/dccl-ids.md#core"
    assert links().resolve("../../core/src/doc/apps/pViewMessage.md", "tool/build.md") == \
        "../core/apps/pViewMessage.md"


def test_link_to_unpublished_file_goes_to_github():
    assert links().resolve("../../bin/pViewMessage/ViewMessage.cpp",
                           "core/apps/pViewMessage.md") == \
        f"{URL}/blob/abc123/core/src/bin/pViewMessage/ViewMessage.cpp"
    assert links().resolve("../../bin/pViewMessage/", "core/apps/pViewMessage.md") == \
        f"{URL}/tree/abc123/core/src/bin/pViewMessage"
    assert links().resolve("../AGENTS.md", "index.md") == f"{URL}/blob/abc123/AGENTS.md"


def test_submodule_files_link_to_their_own_repository():
    assert links().resolve("repo:icex/src/bin/pIcex/Icex.cpp", "index.md") == \
        "https://github.com/GobySoft/lamss3-icex/blob/def456/src/bin/pIcex/Icex.cpp"


def test_staged_relative_links_to_generated_pages_are_kept():
    assert links().resolve("../reference/cli.md#lamss-build", "tool/build.md") == \
        "../reference/cli.md#lamss-build"


def test_external_and_anchor_links_are_kept():
    for target in ["https://goby.software", "#purpose", "mailto:a@b.c"]:
        assert links().resolve(target, "index.md") == target


def test_repo_scheme_from_generated_page():
    assert links().resolve("repo:core/src/lib/protobuf/lamss/DCCL_IDS.md", "core/index.md") == \
        "../reference/dccl-ids.md"


def test_code_is_not_rewritten():
    text = ("See [x](../AGENTS.md) and `[y](../AGENTS.md)`.\n"
            "```\n[z](../AGENTS.md)\n```\n"
            "[`code` text](../AGENTS.md)\n")
    out = rewrite_markdown(text, "index.md", links())
    lines = out.splitlines()
    assert lines[0] == f"See [x]({URL}/blob/abc123/AGENTS.md) and `[y](../AGENTS.md)`."
    assert lines[2] == "[z](../AGENTS.md)"
    assert lines[4] == f"[`code` text]({URL}/blob/abc123/AGENTS.md)"
