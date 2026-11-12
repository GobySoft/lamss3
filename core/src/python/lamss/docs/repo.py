# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""What the site needs to know about the checkout: active toolboxes and git state."""

import re
import subprocess
from pathlib import Path

# Site order; other toolboxes follow alphabetically.
IN_TREE_ORDER = ["core", "autonomy", "sonar", "virtual_ocean", "gui", "mit"]
DEFAULT_URL = "https://github.com/GobySoft/lamss3"


def site_toolboxes(root):
    """Every in-tree toolbox, whether active or not, and the active submodule toolboxes.

    In-tree pages are always in the checkout, so the site does not depend on what is built.
    """
    root = Path(root)
    names = {tb for tb in IN_TREE_ORDER if (root / tb).is_dir()}
    listed = root / ".lamss" / "toolboxes"
    if listed.is_file():
        for line in listed.read_text().splitlines():
            line = line.split("#", 1)[0].strip()
            if line:
                names.add(line)
    known = [tb for tb in IN_TREE_ORDER if tb in names]
    return known + sorted(names - set(known))


def toolbox_requires(root, toolbox):
    cmake = Path(root) / toolbox / "CMakeLists.txt"
    if not cmake.is_file():
        return []
    match = re.search(r"lamss_toolbox\s*\(([^)]*)\)", cmake.read_text())
    if not match:
        return []
    words = match.group(1).split()
    if "REQUIRES" not in words:
        return []
    requires = []
    for word in words[words.index("REQUIRES") + 1:]:
        if word.isupper():
            break
        requires.append(word)
    return requires


def _git(cwd, *args):
    result = subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True)
    return result.stdout.strip() if result.returncode == 0 else ""


def github_url(remote):
    """https://github.com/<owner>/<repo> for a GitHub remote URL (https, ssh or scp form)."""
    match = re.match(r"^(?:https://|ssh://git@|git@)github\.com[/:]([^/]+)/(.+?)(?:\.git)?/?$", remote)
    return f"https://github.com/{match.group(1)}/{match.group(2)}" if match else ""


def repository_info(path):
    """(url, ref, files) of the git repository at path; ref is the commit, so links stay valid."""
    url = github_url(_git(path, "remote", "get-url", "origin")) or DEFAULT_URL
    ref = _git(path, "rev-parse", "HEAD") or "main"
    files = set(_git(path, "ls-files", "--cached", "--others", "--exclude-standard").splitlines())
    return url, ref, files


def is_submodule(root, toolbox):
    return (Path(root) / toolbox / ".git").is_file()
