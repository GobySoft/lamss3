# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Collects docs/ and every active toolbox's src/doc/ into one tree and generates the rest."""

import json
import os
import shutil
from pathlib import Path

import yaml

from . import cli_reference, messages, overview
from .nav import Nav
from .repo import is_submodule, repository_info, site_toolboxes
from .tree import StagedTree

SITE_FILES = {"mkdocs.yml", "nav.yml", "requirements.txt"}
SITE_DIRS = {"hooks"}
# Not published; links to them go to GitHub.
TOOLBOX_DIRS_NOT_PUBLISHED = {"templates"}
DCCL_IDS = "core/src/lib/protobuf/lamss/DCCL_IDS.md"


def find_lamss(root):
    prefix = os.environ.get("LAMSS_PREFIX") or str(Path(root) / "build" / "usr")
    candidate = Path(prefix) / "bin" / "lamss"
    return str(candidate) if candidate.is_file() else shutil.which("lamss")


class Stage:
    def __init__(self, root, out):
        self.root = Path(root).resolve()
        self.out = Path(out).resolve()
        self.docs = StagedTree(self.out / "docs")
        self.pages = {}
        self.warnings = []
        self.toolboxes = site_toolboxes(self.root)
        self.generated = set()

    def _copy_tree(self, source, staged_prefix, repo_prefix, skip_dirs=(), skip_files=()):
        for path in sorted(source.rglob("*")):
            rel = path.relative_to(source)
            if path.is_dir() or rel.parts[0] in skip_dirs or rel.as_posix() in skip_files:
                continue
            if "__pycache__" in rel.parts:
                continue
            staged = f"{staged_prefix}{rel.as_posix()}"
            self.docs.copy(path, staged)
            self.pages[staged] = f"{repo_prefix}{rel.as_posix()}"

    def collect(self):
        self._copy_tree(self.root / "docs", "", "docs/", SITE_DIRS, SITE_FILES)
        for tb in self.toolboxes:
            source = self.root / tb / "src" / "doc"
            if source.is_dir():
                self._copy_tree(source, f"{tb}/", f"{tb}/src/doc/", TOOLBOX_DIRS_NOT_PUBLISHED)

    def generate(self):
        if (self.root / DCCL_IDS).is_file():
            self.docs.copy(self.root / DCCL_IDS, "reference/dccl-ids.md")
            self.pages["reference/dccl-ids.md"] = DCCL_IDS

        try:
            descriptors = messages.compile_descriptors(
                self.root, self.toolboxes, [Path(os.environ.get("LAMSS_PREFIX", "")) / "include"])
        except RuntimeError as error:
            self.warnings.append(f"message reference not generated: {error}")
            descriptors = None
        if descriptors is not None:
            ids, with_messages = [], []
            for tb in self.toolboxes:
                if not messages.library_protos(self.root, tb)[1]:
                    continue
                text, found = messages.render_toolbox(descriptors, self.root, tb)
                self.docs.write_text(f"reference/messages/{tb}.md", text)
                ids += found
                with_messages.append(tb)
            self.docs.write_text("reference/messages/index.md",
                                 messages.render_index(ids, with_messages))

        lamss = find_lamss(self.root)
        if lamss:
            self.docs.write_text("reference/cli.md", cli_reference.render(lamss))
        else:
            self.warnings.append("command reference not generated: the lamss tool is not built")

        for tb in self.toolboxes:
            if f"{tb}/index.md" not in self.docs.written:
                self.docs.write_text(f"{tb}/index.md",
                                     overview.render(self.root, tb, self.docs.written))
                # where an authored overview would be, so pages can link to it
                self.pages[f"{tb}/index.md"] = f"{tb}/src/doc/index.md"
                self.generated.add(f"{tb}/index.md")

    def _repositories(self):
        url, ref, files = repository_info(self.root)
        repositories = [{"prefix": "", "url": url, "ref": ref, "files": sorted(files)}]
        for tb in self.toolboxes:
            if is_submodule(self.root, tb):
                url, ref, files = repository_info(self.root / tb)
                repositories.append({"prefix": f"{tb}/", "url": url, "ref": ref,
                                     "files": sorted(files)})
        return repositories

    def write_config(self, site_dir, serve=False):
        nav_source = yaml.safe_load((self.root / "docs" / "nav.yml").read_text())
        nav = Nav(self.docs.written, self.toolboxes).expand(nav_source)
        config = {
            "INHERIT": str(self.root / "docs" / "mkdocs.yml"),
            "docs_dir": "docs",
            "site_dir": str(site_dir),
            "hooks": [str(self.root / "docs" / "hooks" / "links.py")],
            "nav": nav,
        }
        if serve:
            config["watch"] = [str(self.root / "docs")] + [
                str(self.root / tb / "src" / "doc") for tb in self.toolboxes
                if (self.root / tb / "src" / "doc").is_dir()]
        links = {"pages": self.pages, "generated": sorted(self.generated),
                 "repositories": self._repositories()}
        _write_if_changed(self.out / "lamss-links.json", json.dumps(links, indent=1))
        # JSON is YAML, and mkdocs.yml must not be rewritten unchanged while `serve` watches it
        _write_if_changed(self.out / "mkdocs.yml", json.dumps(config, indent=2))
        return self.out / "mkdocs.yml"

    def run(self, site_dir, serve=False):
        self.collect()
        self.generate()
        self.docs.remove_stale()
        return self.write_config(site_dir, serve)


def _write_if_changed(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.is_file() and path.read_text() == text:
        return
    path.write_text(text)
