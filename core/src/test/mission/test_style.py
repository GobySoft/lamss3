# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""The lamss tool's text styles, as `lamss mission` uses them."""

import io

from lamss import style
from lamss.mission.cli import parser


class Terminal(io.StringIO):
    def isatty(self):
        return True


def test_styles_only_on_a_terminal(monkeypatch):
    monkeypatch.delenv("NO_COLOR", raising=False)
    monkeypatch.setenv("TERM", "xterm")
    assert style.good("ok", io.StringIO()) == "ok"
    assert style.good("ok", Terminal()) == "\033[32mok\033[0m"
    assert style.error("error:", Terminal()) == "\033[1;31merror:\033[0m"
    monkeypatch.setenv("NO_COLOR", "1")
    assert style.good("ok", Terminal()) == "ok"


def test_help_uses_the_tool_headings():
    text = parser().format_help()
    assert text.startswith("Usage: lamss mission")
    assert "\nCommands:\n  list " in text
    assert "<command>\n" not in text.split("Commands:")[1]
    assert "\nOptions:\n" in text
    show = parser()._subparsers._group_actions[0].choices["show"].format_help()
    assert "\nArguments:\n  cruise " in show
