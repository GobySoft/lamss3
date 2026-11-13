# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later
"""The lamss tool's text styles (core/src/bin/lamss/style.cpp), for its Python commands.

One fixed meaning per style; plain text unless the stream is a terminal, NO_COLOR is unset and
TERM is not dumb.
"""

import argparse
import os
import re
import sys


def enabled(stream=None):
    stream = stream or sys.stdout
    if os.environ.get("NO_COLOR") is not None or os.environ.get("TERM") == "dumb":
        return False
    return hasattr(stream, "isatty") and stream.isatty()


def _wrap(code, text, stream):
    return f"\033[{code}m{text}\033[0m" if enabled(stream) else text


def heading(text, stream=None):
    return _wrap("1", text, stream)


def command(text, stream=None):
    return _wrap("36", text, stream)


def good(text, stream=None):
    return _wrap("32", text, stream)


def attention(text, stream=None):
    return _wrap("33", text, stream)


def error(text, stream=None):
    return _wrap("1;31", text, stream or sys.stderr)


def note(text, stream=None):
    return _wrap("2", text, stream)


_SECTIONS = {"positional arguments": "Arguments", "options": "Options",
             "optional arguments": "Options"}
_SECTION = re.compile(r"^(\S[^\n]*):$", re.M)


class HelpFormatter(argparse.HelpFormatter):
    """argparse help laid out with the lamss tool's headings: Usage, Commands, Arguments, Options."""

    def format_help(self):
        text = super().format_help()
        # subcommands listed directly under Commands, without argparse's metavar line
        text = re.sub(r"^  <command>\n((?:    .*\n)+)",
                      lambda m: re.sub(r"^  ", "", m.group(1), flags=re.M), text, flags=re.M)
        if text.startswith("usage: "):
            text = heading("Usage:") + " " + text[len("usage: "):]

        def section(match):
            name = _SECTIONS.get(match.group(1), match.group(1))
            return heading(f"{name}:")

        return _SECTION.sub(section, text)


class ArgumentParser(argparse.ArgumentParser):
    def __init__(self, *args, **kwargs):
        kwargs.setdefault("formatter_class", HelpFormatter)
        if sys.version_info >= (3, 14):
            kwargs.setdefault("color", False)
        super().__init__(*args, **kwargs)

    def error(self, message):
        self.print_usage(sys.stderr)
        self.exit(2, f"{self.prog}: {error('error:')} {message}\n")


def print_error(prog, message):
    print(f"{prog}: {error('error:')} {message}", file=sys.stderr, flush=True)


def print_note(message):
    print(f"{attention('note:', sys.stderr)} {message}", file=sys.stderr)
