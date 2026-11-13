# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Finds the publish/subscribe and configuration calls in C++ sources, without a compiler.

Regular expressions over comment-stripped source, so it misses what a parser would not: names
built at run time are reported as expressions for a person to resolve.
"""

import re
from dataclasses import dataclass

_STRING = re.compile(r'"(?:[^"\\]|\\.)*"')
_NUMBER = re.compile(r"^[-+]?(\d+\.?\d*|\.\d+)([eE][-+]?\d+)?[fFlL]?$")
_DECLARATION = re.compile(r"^(const\s+)?[\w:<>,\s]+[\s&*]+\w+$")

_MOOS_CALL = re.compile(
    r"(?:\b(?:m_Comms|Comms)\s*\.\s*|\bthis\s*->\s*|(?<![\w.>:]))"
    r"(?P<fn>Register|Notify|subscribe_pb|subscribe|publish_pb|publish|addInfoVars|"
    r"postMessage|postIntMessage|postBoolMessage|postRepeatableMessage|"
    r"getBufferDoubleVal|getBufferStringVal|getBufferVal)\s*(?P<template><[^;()]*?>)?\s*\(")
_GOBY_CALL = re.compile(
    r"\b(?P<layer>interprocess|intervehicle|interthread)\s*\(\s*\)\s*\.\s*"
    r"(?P<fn>subscribe|publish)\s*<")
_CONFIG_CALL = re.compile(
    r"\b(?:m_MissionReader\s*\.\s*)?(?P<fn>GetConfigurationParam|GetValue)\s*\(")
_PARAM_COMPARE = re.compile(r'\b(?:param|sParam|param_name)\s*==\s*"([^"]+)"')
_MAIL_COMPARE = re.compile(r'\b(?:\w+\s*\.\s*)?(?:m_sKey|GetKey\s*\(\s*\))\s*==\s*"([^"]+)"')

SUBSCRIBE = {"Register", "subscribe", "subscribe_pb", "addInfoVars", "getBufferDoubleVal",
             "getBufferStringVal", "getBufferVal"}
PUBLISH = {"Notify", "publish", "publish_pb", "postMessage", "postIntMessage",
           "postBoolMessage", "postRepeatableMessage"}


@dataclass
class Use:
    direction: str  # subscribes, publishes or config
    name: str  # variable, group or parameter; "?<expression>" when built at run time
    transport: str
    type: str
    where: str  # handler function, or file
    note: str = ""


def strip_comments(text):
    """Blanks comments (keeping line breaks) and leaves strings alone."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1])
            i = j + 1
        elif text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            out.append(" " * (j - i))
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(re.sub(r"[^\n]", " ", text[i:j]))
            i = j
        else:
            out.append(c)
            i += 1
    return "".join(out)


def arguments(text, start, close=")"):
    """Top-level arguments of the call whose opening bracket is just before `start`."""
    opening = {")": "(", ">": "<"}[close]
    args, depth, current, i = [], 0, [], start
    while i < len(text):
        c = text[i]
        if c == '"':
            match = _STRING.match(text, i)
            end = match.end() if match else i + 1
            current.append(text[i:end])
            i = end
            continue
        if c in "([{" or (c == "<" and close == ">"):
            depth += 1
        elif c in ")]}" or (c == ">" and close == ">"):
            if depth == 0 and c == close:
                args.append("".join(current).strip())
                return [" ".join(a.split()) for a in args if a]
            depth -= 1
        elif c == "," and depth == 0:
            args.append("".join(current).strip())
            current = []
            i += 1
            continue
        current.append(c)
        i += 1
    return []


def names(expression):
    """Variable names a first argument can take: its string literals, unless it builds one."""
    literals = [s[1:-1] for s in _STRING.findall(expression)]
    if literals and "+" not in expression and "<<" not in expression:
        return literals, ""
    return [f"?{expression}"], "name built at run time"


_DECLARED = r"\b(double|float|int|long|unsigned|bool|std::string|string)\s+{}\s*[;=,)]"


def value_type(expression, text=""):
    if not expression:
        return ""
    if re.fullmatch(r"\w+", expression):
        match = re.search(_DECLARED.format(re.escape(expression)), text)
        if match:
            return "string" if "string" in match.group(1) else "double"
    if _STRING.fullmatch(expression) or expression.endswith((".str()", ".c_str()")) \
            or expression.startswith(("std::string(", "string(")):
        return "string"
    if _NUMBER.match(expression) or expression.startswith(("(double)", "double(")):
        return "double"
    if expression in ("true", "false"):
        return "string (true/false)"
    return ""


def _line(text, index):
    return text.count("\n", 0, index) + 1


def _handler(argument):
    match = re.match(r"^&\s*(?:\w+::)*(\w+)$", argument or "")
    return match.group(1) if match else ""


def scan(path, text):
    """Every Use in one source file."""
    text = strip_comments(text)
    file = path.name
    uses = []
    for match in _MOOS_CALL.finditer(text):
        fn = match.group("fn")
        args = arguments(text, match.end())
        if not args or _DECLARATION.match(args[0]) and not _STRING.search(args[0]):
            continue
        direction = "subscribes" if fn in SUBSCRIBE else "publishes"
        if fn == "addInfoVars":
            literals = [s[1:-1] for s in _STRING.findall(args[0])]
            found = [n.strip() for lit in literals for n in lit.split(",") if n.strip()]
            note = ""
            if not found:
                found, note = names(args[0])
            for name in found:
                uses.append(Use(direction, name, "MOOS (helm)", "", file, note))
            continue
        found, note = names(args[0])
        if fn.endswith("_pb"):
            template = (match.group("template") or "").strip("<> ")
            kind = f"protobuf {template}".strip()
        elif fn.startswith("getBuffer"):
            kind = {"getBufferDoubleVal": "double", "getBufferStringVal": "string"}.get(fn, "")
        elif fn == "postIntMessage":
            kind = "double (integer)"
        elif fn == "postBoolMessage":
            kind = "string (true/false)"
        elif direction == "publishes":
            kind = value_type(args[1] if len(args) > 1 else "", text)
        else:
            kind = ""
        transport = "MOOS (helm)" if fn.startswith(("post", "getBuffer")) else "MOOS"
        where = (_handler(args[1]) if fn.startswith("subscribe") and len(args) > 1 else "") or file
        for name in found:
            uses.append(Use(direction, name, transport, kind, where, note))

    for match in _GOBY_CALL.finditer(text):
        template = arguments(text, match.end(), close=">")
        if not template:
            continue
        direction = "subscribes" if match.group("fn") == "subscribe" else "publishes"
        group = template[0]
        kind = template[1] if len(template) > 1 else ""
        uses.append(Use(direction, group, f"Goby {match.group('layer')}", kind, file))

    for match in _CONFIG_CALL.finditer(text):
        args = arguments(text, match.end())
        if not args or not _STRING.fullmatch(args[0]):
            continue
        note = "global (mission file)" if match.group("fn") == "GetValue" else ""
        uses.append(Use("config", args[0][1:-1], "", "", file, note))

    for match in _PARAM_COMPARE.finditer(text):
        uses.append(Use("config", match.group(1), "", "", file))
    for match in _MAIL_COMPARE.finditer(text):
        uses.append(Use("subscribes", match.group(1), "MOOS", "", "OnNewMail"))
    return uses
