# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""A toolbox's overview page, generated when the toolbox has no authored doc/index.md."""

from pathlib import Path

import yaml

from .repo import toolbox_requires


def _apt(root, toolbox):
    path = Path(root) / toolbox / "dependencies.yml"
    if not path.is_file():
        return {}
    data = yaml.safe_load(path.read_text()) or {}
    return data.get("apt") or {}


def _entries(root, toolbox, kind):
    base = Path(root) / toolbox / "src" / kind
    if not base.is_dir():
        return []
    return sorted(p.name for p in base.iterdir() if p.is_dir() and p.name != "protobuf")


def render(root, toolbox, documented):
    """Markdown for the staged <toolbox>/index.md.

    documented: staged paths of the toolbox's pages, to link apps and libraries to their pages.
    """
    requires = toolbox_requires(root, toolbox)
    lines = [f"# {toolbox}", "",
             "This overview is generated from the toolbox's `CMakeLists.txt`, "
             f"`dependencies.yml` and source directories. Add `{toolbox}/src/doc/index.md` to "
             "replace it with an authored one.", ""]
    if toolbox != "core":
        lines += [f"- **Requires:** {', '.join(f'[{r}](../{r}/index.md)' for r in requires) or 'nothing'}",
                  f"- **Activate:** `lamss toolbox activate {toolbox}`", ""]
    for kind, title, folder in (("bin", "Applications", "apps"), ("lib", "Libraries", "libraries")):
        names = _entries(root, toolbox, kind)
        if not names:
            continue
        lines += [f"## {title}", ""]
        for name in names:
            page = f"{toolbox}/{folder}/{name}.md"
            if page in documented:
                lines.append(f"- [{name}]({folder}/{name}.md)")
            else:
                lines.append(f"- [{name}](repo:{toolbox}/src/{kind}/{name}) (no page yet)")
        lines.append("")
    if (Path(root) / toolbox / "src" / "lib" / "protobuf").is_dir():
        lines += ["## Messages", "", f"See [{toolbox} messages](../reference/messages/{toolbox}.md).", ""]
    apt = _apt(root, toolbox)
    if apt:
        lines += ["## Dependencies", "", "Installed by `lamss dependencies install`:", ""]
        for release, packages in apt.items():
            label = "every release" if release == "all" else release
            lines.append(f"- {label}: " + ", ".join(f"`{p}`" for p in packages or []))
        lines.append("")
    return "\n".join(lines) + "\n"
