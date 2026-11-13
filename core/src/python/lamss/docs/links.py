# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Rewrites relative Markdown links written against the repository so they work on the site.

Pages are written to be read on GitHub, so their links are relative to the page's place in the
repository. On the site a link to another published page becomes a link to its staged copy, and a
link to anything else in the repository (source code, AGENTS.md) becomes a GitHub URL.
"""

import posixpath
import re
from dataclasses import dataclass, field

_FENCE = re.compile(r"^\s*(`{3,}|~{3,})")
_INLINE_CODE = re.compile(r"(`+).+?\1")
_LINK = re.compile(r"(!?\[[^\]]*\]\()([^)\s]+)((?:\s+\"[^\"]*\")?\))")
_EXTERNAL = re.compile(r"^([a-zA-Z][a-zA-Z0-9+.-]*:|//|#|/)")


@dataclass
class Repository:
    """A git repository whose files appear under `prefix` in the lamss3 tree (a submodule toolbox
    has its own)."""

    prefix: str
    url: str
    ref: str
    files: set = field(default_factory=set)
    dirs: set = field(default_factory=set)

    def __post_init__(self):
        for path in self.files:
            parent = posixpath.dirname(path)
            while parent and parent not in self.dirs:
                self.dirs.add(parent)
                parent = posixpath.dirname(parent)


class LinkMap:
    def __init__(self, pages, repositories, staged_files=()):
        # pages: staged path -> repository path (real or, for generated pages, where it would be)
        self.pages = dict(pages)
        self.sources = {repo: staged for staged, repo in self.pages.items()}
        self.repositories = sorted(repositories, key=lambda r: len(r.prefix), reverse=True)
        self.staged_files = set(staged_files)

    def _url(self, repo_path):
        for repo in self.repositories:
            if repo.prefix and not repo_path.startswith(repo.prefix):
                continue
            inner = repo_path[len(repo.prefix):]
            if inner in repo.files:
                return f"{repo.url}/blob/{repo.ref}/{inner}"
            if inner in repo.dirs or inner == "":
                return f"{repo.url}/tree/{repo.ref}/{inner}".rstrip("/")
        return None

    def edit_url(self, repo_path):
        for repo in self.repositories:
            if repo_path.startswith(repo.prefix):
                return f"{repo.url}/edit/main/{repo_path[len(repo.prefix):]}"
        return None

    def resolve(self, target, page):
        """The site's link for `target` as written in staged page `page`.

        Generated pages link to repository files as `repo:<path from the repository root>`.
        """
        path, sep, anchor = target.partition("#")
        here = posixpath.dirname(page)
        if path.startswith("repo:"):
            repo_path = posixpath.normpath(path[len("repo:"):])
            if repo_path in self.sources:
                return posixpath.relpath(self.sources[repo_path], here or ".") + sep + anchor
            url = self._url(repo_path)
            return (url + sep + anchor) if url else target
        if _EXTERNAL.match(target):
            return target

        source = self.pages.get(page)
        if source is not None:
            repo_path = posixpath.normpath(posixpath.join(posixpath.dirname(source), path))
            if repo_path in self.sources:
                staged = self.sources[repo_path]
                return posixpath.relpath(staged, here or ".") + sep + anchor
            if not repo_path.startswith(".."):
                staged = posixpath.normpath(posixpath.join(here, path))
                if staged not in self.staged_files:
                    url = self._url(repo_path)
                    if url:
                        return url + sep + anchor
        return target


def rewrite_markdown(text, page, links):
    """Rewrites every link outside code blocks and inline code."""
    out = []
    fence = None
    for line in text.splitlines(keepends=True):
        match = _FENCE.match(line)
        if fence:
            if match and match.group(1)[0] == fence[0] and len(match.group(1)) >= len(fence):
                fence = None
            out.append(line)
            continue
        if match:
            fence = match.group(1)
            out.append(line)
            continue
        out.append(_rewrite_line(line, page, links))
    return "".join(out)


def _rewrite_line(line, page, links):
    code = [m.span() for m in _INLINE_CODE.finditer(line)]

    def replace(match):
        if any(start <= match.start() < end for start, end in code):
            return match.group(0)
        return match.group(1) + links.resolve(match.group(2), page) + match.group(3)

    return _LINK.sub(replace, line)
