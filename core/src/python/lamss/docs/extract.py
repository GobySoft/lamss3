# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fills the generated interface tables of inferred pages from the code.

Between `<!-- BEGIN GENERATED <table> -->` and `<!-- END GENERATED <table> -->`, rows come from
the sources named by the page's `source`. Meaning and Notes, and any column the code does not
determine, keep what the page already says, matched by name.
"""

import re
import subprocess
import tempfile
from pathlib import Path

import yaml

from . import cpp

TABLES = {
    "subscribes": ["Name", "Transport", "Type / format", "Handler", "Meaning", "Notes"],
    "publishes": ["Name", "Transport", "Type / format", "Where", "Meaning", "Notes"],
    "config": ["Parameter", "Type", "Default", "Required", "Meaning"],
}
KEPT = {"Meaning", "Notes"}
_BLOCK = re.compile(r"(<!-- BEGIN GENERATED (\w+) -->\n)(.*?)(<!-- END GENERATED \2 -->)", re.S)
_FRONT_MATTER = re.compile(r"\A---\n(.*?)\n---\n", re.S)
SOURCE_SUFFIXES = {".cpp", ".cc", ".h", ".hpp"}


def front_matter(text):
    match = _FRONT_MATTER.match(text)
    return (yaml.safe_load(match.group(1)) or {}) if match else {}


def _code(name):
    return f"`{name[1:]}` (built at run time)" if name.startswith("?") else f"`{name}`"


def _join(values):
    seen = []
    for v in values:
        if v and v not in seen:
            seen.append(v)
    return ", ".join(seen)


def interface_rows(source_dir, config_rows=None):
    """{table: [row dict]} for the sources in source_dir."""
    uses = []
    for path in sorted(Path(source_dir).rglob("*")):
        if path.suffix in SOURCE_SUFFIXES and path.is_file():
            uses += cpp.scan(path, path.read_text(errors="replace"))
    params = {u.name for u in uses if u.direction == "config"}
    rows = {"subscribes": {}, "publishes": {}, "config": {}}
    for use in uses:
        if use.direction == "config":
            continue
        key = use.name
        note = use.note
        if key.startswith("?") and key[1:] in params:
            note = f"named by the `{key[1:]}` parameter"
        row = rows[use.direction].setdefault(key, {"Name": _code(key), "_t": [], "_ty": [],
                                                   "_w": [], "_n": []})
        row["_t"].append(use.transport)
        row["_ty"].append(use.type)
        row["_w"].append(use.where)
        row["_n"].append(note)
    out = {}
    for table in ("subscribes", "publishes"):
        out[table] = []
        for row in rows[table].values():
            where = "Handler" if table == "subscribes" else "Where"
            out[table].append({
                "Name": row["Name"], "Transport": _join(row["_t"]),
                "Type / format": _join(row["_ty"]),
                where: _join(f"`{w}`" for w in row["_w"]), "Notes": _join(row["_n"])})
    if config_rows is not None:
        out["config"] = config_rows
    else:
        out["config"] = []
        for use in uses:
            if use.direction == "config" and use.name not in rows["config"]:
                rows["config"][use.name] = True
                out["config"].append({"Parameter": f"`{use.name}`", "Meaning": use.note})
    return out


def config_proto_rows(root, source_dir):
    """Rows for the app's configuration message, when it has a config proto."""
    protos = sorted(Path(source_dir).glob("*.proto"))
    if not protos:
        return None
    from . import messages

    includes = [str(source_dir)] + [str(p) for p in Path(root).glob(f"*/{messages.PROTO_DIR}")]
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "set.pb"
        command = ["protoc", "--include_imports", f"--descriptor_set_out={out}"] + \
            [f"-I{i}" for i in includes] + ["-I/usr/include"] + [p.name for p in protos]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError(f"protoc failed on {source_dir}:\n{result.stderr}")
        descriptor_set = messages.descriptor_pb2.FileDescriptorSet.FromString(out.read_bytes())
    names = {p.name for p in protos}
    files = [f for f in descriptor_set.file if f.name in names]
    options = messages.Options(descriptor_set)
    config = None
    for file in files:
        for message in file.message_type:
            if any(f.type_name.endswith("GobyMOOSAppConfig") or f.type_name.endswith("AppConfig")
                   for f in message.field):
                config = (file, message)
                break
        if config:
            break
    if not config:
        return None
    file, message = config
    local = {f".{file.package}.{m.name}": m for m in file.message_type}

    def rows(message, prefix, depth):
        out = []
        for field in message.field:
            name = prefix + field.name
            type_name = messages._type_name(field)
            description = options.field(field, "goby.field").get("description", "")
            out.append({
                "Parameter": f"`{name}`",
                "Type": f"{'repeated ' if field.label == 3 else ''}{type_name.split('.')[-1]}",
                "Default": f"`{field.default_value}`" if field.HasField("default_value") else "",
                "Required": "yes" if field.label == 2 else "no",
                "Meaning": description,
            })
            nested = local.get(field.type_name)
            if nested is not None and depth < 2:
                out += rows(nested, f"{name}.", depth + 1)
        return out

    return rows(message, "", 0)


def _parse_table(body):
    lines = [l for l in body.strip().splitlines() if l.startswith("|")]
    if len(lines) < 2:
        return []
    header = [c.strip() for c in _split(lines[0])]
    return [dict(zip(header, (c.strip() for c in _split(line)))) for line in lines[2:]]


def _split(line):
    cells, current, escaped = [], [], False
    for c in line.strip()[1:-1]:
        if c == "|" and not escaped:
            cells.append("".join(current))
            current = []
        else:
            current.append(c)
        escaped = c == "\\" and not escaped
    cells.append("".join(current))
    return cells


def _key(cell):
    return re.sub(r"\*\*\[\w+\]\*\*|`|\s+", "", cell.split("(built")[0]).strip()


def render_table(table, rows, old_rows):
    columns = TABLES[table]
    old = {_key(r.get(columns[0], "")): r for r in old_rows}
    lines = ["| " + " | ".join(columns) + " |", "|" + "---|" * len(columns)]
    for row in rows:
        previous = old.get(_key(row[columns[0]]), {})
        cells = []
        for column in columns:
            generated = row.get(column, "")
            kept = previous.get(column, "")
            if column in KEPT or (kept and kept.startswith(generated)):
                cells.append(kept or generated)
            else:
                cells.append(generated or kept)
        lines.append("| " + " | ".join(c.replace("\n", " ") for c in cells) + " |")
    return "\n".join(lines) + "\n"


def update_page(root, page_path):
    """The page's text with its generated blocks refreshed."""
    text = Path(page_path).read_text()
    meta = front_matter(text)
    source = Path(root) / str(meta.get("source", "")).rstrip("/")
    if not meta.get("source") or not source.is_dir():
        raise RuntimeError(f"{page_path}: front matter `source` is not a directory")
    tables = interface_rows(source, config_proto_rows(root, source))

    def replace(match):
        table = match.group(2)
        if table not in TABLES:
            return match.group(0)
        body = render_table(table, tables.get(table, []), _parse_table(match.group(3)))
        return match.group(1) + body + match.group(4)

    return _BLOCK.sub(replace, text)


def generated_pages(root):
    for page in sorted(Path(root).glob("*/src/doc/**/*.md")):
        text = page.read_text()
        if "<!-- BEGIN GENERATED" in text and "templates" not in page.parts:
            yield page


TEMPLATE = "core/src/doc/templates/inferred-app.md"
_FOLDERS = {"ivp-behavior": "behaviors", "library": "libraries"}


def kind_of(source_dir):
    text = " ".join(p.read_text(errors="replace") for p in Path(source_dir).rglob("*")
                    if p.suffix in SOURCE_SUFFIXES and p.is_file())
    if "IvPBehavior" in text:
        return "ivp-behavior"
    if "GobyMOOSApp" in text:
        return "goby-moos-app"
    if "CMOOSApp" in text:
        return "moos-app"
    if re.search(r"goby::middleware::\w*Application|goby::zeromq::\w*Application", text):
        return "goby-app"
    if "/src/bin/" in Path(source_dir).as_posix() + "/":
        return "app"
    return "library"


def new_page(root, source, by="<agent>"):
    """(path, text) of a new inferred page for the code in `source` (relative to root)."""
    root = Path(root)
    source = Path(source)
    if source.is_absolute():
        source = source.relative_to(root)
    toolbox = source.parts[0]
    name = source.name
    kind = kind_of(root / source)
    folder = _FOLDERS.get(kind, "apps")
    commit = subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=root,
                            capture_output=True, text=True).stdout.strip()
    text = (root / TEMPLATE).read_text()
    text = re.sub(r"\A---\n.*?\n---\n", "", text, flags=re.S)
    meta = (f"---\ntitle: {name}\ntoolbox: {toolbox}\nkind: {kind}\n"
            f"source: {source.as_posix()}/\ninferred_by: {by}\ninferred_from: {commit}\n"
            "status: inferred\nreviewed_by:\nreviewed_on:\nlaunched_by: []\n---\n")
    text = meta + text.replace("<app name>", name).replace("`<commit>`", f"`{commit}`") \
        .replace("<agent>", by)
    text = re.sub(r"(<!-- BEGIN GENERATED (\w+) -->\n).*?(<!-- END GENERATED \2 -->)",
                  lambda m: m.group(1) + m.group(3), text, flags=re.S)
    return root / toolbox / "src" / "doc" / folder / f"{name}.md", text
