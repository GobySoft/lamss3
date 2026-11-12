# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Expands docs/nav.yml into the site's nav for the staged pages.

Entries for pages that were not staged (an inactive toolbox) are dropped. Placeholders:
"@toolboxes" (a section per active toolbox), "@messages" (the generated message pages) and
"@dir:<staged dir>" (the pages under a directory, index or README first).
"""

import posixpath

TOOLBOX_SECTIONS = ["guide", "missions", "apps", "libraries", "behaviors"]
TOOLBOX_EXCLUDED = {"plan"}
_INDEX = ("index.md", "README.md")


def _title(name):
    return name.replace("_", " ").replace("-", " ").capitalize()


class Nav:
    def __init__(self, staged, toolboxes):
        self.pages = sorted(p for p in staged if p.endswith(".md"))
        self.toolboxes = toolboxes

    def _children(self, directory):
        prefix = f"{directory}/" if directory else ""
        files, dirs = [], set()
        for page in self.pages:
            if not page.startswith(prefix):
                continue
            rest = page[len(prefix):]
            if "/" in rest:
                dirs.add(rest.split("/", 1)[0])
            else:
                files.append(rest)
        files.sort(key=lambda f: (f not in _INDEX, f))
        return [prefix + f for f in files], sorted(dirs)

    def directory(self, directory, order=(), excluded=()):
        files, dirs = self._children(directory)
        dirs = [d for d in order if d in dirs] + [d for d in dirs if d not in order]
        entries = list(files)
        for d in dirs:
            if d in excluded:
                continue
            sub = self.directory(posixpath.join(directory, d))
            if sub:
                entries.append({_title(d): sub})
        return entries

    def placeholder(self, value):
        if value == "@toolboxes":
            entries = []
            for tb in self.toolboxes:
                sub = self.directory(tb, TOOLBOX_SECTIONS, TOOLBOX_EXCLUDED)
                if sub:
                    entries.append({tb: sub})
            return entries
        if value == "@messages":
            entries = ["reference/messages/index.md"] if "reference/messages/index.md" in self.pages else []
            return entries + [{tb: f"reference/messages/{tb}.md"} for tb in self.toolboxes
                              if f"reference/messages/{tb}.md" in self.pages]
        if value.startswith("@dir:"):
            return self.directory(value[len("@dir:"):].strip("/"))
        raise ValueError(f"unknown nav placeholder {value}")

    def expand(self, entries):
        out = []
        for entry in entries:
            if isinstance(entry, str):
                if entry.startswith("@"):
                    out += self.placeholder(entry)
                elif entry in self.pages:
                    out.append(entry)
                continue
            ((title, value),) = entry.items()
            if isinstance(value, list):
                items = self.expand(value)
            elif value.startswith("@"):
                items = self.placeholder(value)
            else:
                items = value if value in self.pages else None
            if items:
                out.append({title: items})
        return out
