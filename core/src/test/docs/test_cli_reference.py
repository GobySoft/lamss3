# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

from lamss.docs.cli_reference import subcommands

HELP = """Usage: lamss <command> [options]

Build, configure and run lamss3.

Commands:
  build, b            Configure (if needed), build and install the active toolboxes
  toolbox, tb         List, add, remove, activate and deactivate toolboxes
  env                 Set up shells

Options:
  -h, --help      Show this help
"""


def test_subcommands_are_read_from_the_commands_section():
    assert subcommands(HELP) == ["build", "toolbox", "env"]
    assert subcommands("Usage: lamss build [options]\n\nOptions:\n  -j N  jobs\n") == []
