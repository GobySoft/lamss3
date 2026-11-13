# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""mkdocs hook: repository-relative links and edit links for the staged pages."""

import json
import os
from pathlib import Path

from lamss.docs.links import LinkMap, Repository, rewrite_markdown

_links = None


def on_config(config):
    global _links
    stage = Path(config.config_file_path).parent
    if os.environ.get("LAMSS_DOCS_RESTAGE") == "1":
        # `lamss docs serve`: pick up edits to the source pages before each rebuild
        from lamss.docs.stage import Stage

        Stage(os.environ.get("LAMSS_ROOT", stage.parents[2]), stage).run(config.site_dir, True)
    data = json.loads((stage / "lamss-links.json").read_text())
    repositories = [Repository(r["prefix"], r["url"], r["ref"], set(r["files"]))
                    for r in data["repositories"]]
    _links = LinkMap(data["pages"], repositories)
    _links.generated = set(data["generated"])
    return config


def on_files(files, config):
    _links.staged_files = {f.src_uri for f in files}
    return files


def on_page_markdown(markdown, page, config, files):
    if page.meta.get("status") == "reviewed":
        del page.meta["status"]
    source = _links.pages.get(page.file.src_uri)
    if source and page.file.src_uri not in _links.generated:
        page.edit_url = _links.edit_url(source)
    return rewrite_markdown(markdown, page.file.src_uri, _links)
