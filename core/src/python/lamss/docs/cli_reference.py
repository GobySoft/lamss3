# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""The `lamss` command reference, generated from the tool's own --help text."""

import os
import re
import subprocess

_COMMAND = re.compile(r"^  (\S+?)(?:,\s*\S+)?\s{2,}\S")


def subcommands(help_text):
    """Command names listed under "Commands:" in the tool's help."""
    names = []
    in_commands = False
    for line in help_text.splitlines():
        if line.startswith("Commands:"):
            in_commands = True
            continue
        if in_commands:
            if not line.strip():
                break
            match = _COMMAND.match(line)
            if match:
                names.append(match.group(1))
    return names


def _help(lamss, path):
    env = dict(os.environ, NO_COLOR="1")
    result = subprocess.run([lamss, *path, "--help"], capture_output=True, text=True, env=env,
                            timeout=60)
    return (result.stdout or result.stderr).rstrip()


def render(lamss):
    """Markdown for every command reachable from `lamss --help`."""
    lines = ["# lamss command reference", "",
             "Generated from `lamss <command> --help`. The [user guide](../tool/index.md) explains "
             "how the commands fit together.", ""]

    def walk(path):
        text = _help(lamss, path)
        lines.extend([f"## `{' '.join(['lamss', *path])}`", "", "```text", text, "```", ""])
        for name in subcommands(text):
            walk([*path, name])

    walk([])
    return "\n".join(lines) + "\n"
