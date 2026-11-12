# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Message reference pages, generated from each toolbox's protobuf library.

protoc writes a descriptor set with source comments; the DCCL, Goby and lamss options are read
through a descriptor pool built from that set, so no generated Python code is needed.
"""

import shutil
import subprocess
import tempfile
from pathlib import Path

from google.protobuf import descriptor_pb2, descriptor_pool, json_format, message_factory

PROTO_DIR = "src/lib/protobuf"
_FIELD_EXTENSIONS = {"dccl": "dccl.field", "goby": "goby.field", "lamss": "lamss.core.field"}
_LABEL = {1: "optional", 2: "required", 3: "repeated"}
# FileDescriptorProto field numbers, for source_code_info paths
_MESSAGE, _ENUM, _FIELD, _NESTED, _NESTED_ENUM, _VALUE = 4, 5, 2, 3, 4, 2


def library_protos(root, toolbox):
    base = Path(root) / toolbox / PROTO_DIR
    if not base.is_dir():
        return base, []
    return base, sorted(p.relative_to(base).as_posix() for p in base.rglob("*.proto"))


def compile_descriptors(root, toolboxes, extra_includes=()):
    """FileDescriptorSet of every toolbox's library protos and their imports."""
    protoc = shutil.which("protoc")
    if not protoc:
        raise RuntimeError("protoc not found (apt install protobuf-compiler)")
    includes, protos = [], []
    for tb in toolboxes:
        base, names = library_protos(root, tb)
        if names:
            includes.append(str(base))
            protos += names
    if not protos:
        return descriptor_pb2.FileDescriptorSet()
    includes += [str(p) for p in extra_includes if Path(p).is_dir()]
    includes.append("/usr/include")
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "set.pb"
        command = [protoc, "--include_imports", "--include_source_info",
                   f"--descriptor_set_out={out}"] + [f"-I{i}" for i in includes] + protos
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError("protoc failed:\n" + result.stderr)
        return descriptor_pb2.FileDescriptorSet.FromString(out.read_bytes())


class Options:
    """Reads custom options out of the serialized options in a descriptor set."""

    def __init__(self, descriptor_set):
        self.pool = descriptor_pool.DescriptorPool()
        for file in descriptor_set.file:
            self.pool.Add(file)
        self._classes = {}

    def _parse(self, options, type_name):
        if type_name not in self._classes:
            try:
                descriptor = self.pool.FindMessageTypeByName(type_name)
            except KeyError:
                return None
            self._classes[type_name] = message_factory.GetMessageClass(descriptor)
        return self._classes[type_name].FromString(options.SerializeToString())

    def _extension(self, options, type_name, name):
        if not options.ByteSize():
            return {}
        parsed = self._parse(options, type_name)
        try:
            extension = self.pool.FindExtensionByName(name)
        except KeyError:
            return {}
        if parsed is None or not parsed.HasExtension(extension):
            return {}
        return json_format.MessageToDict(parsed.Extensions[extension],
                                         preserving_proto_field_name=True)

    def field(self, field, name):
        return self._extension(field.options, "google.protobuf.FieldOptions", name)

    def message(self, message, name):
        return self._extension(message.options, "google.protobuf.MessageOptions", name)


def _comments(file):
    comments = {}
    for location in file.source_code_info.location:
        text = " ".join(c.strip() for c in [location.leading_comments, location.trailing_comments]
                        if c.strip())
        if text:
            comments[tuple(location.path)] = " ".join(text.split())
    return comments


def _cell(text):
    return str(text).replace("|", "\\|").replace("\n", " ")


def _units(units):
    parts = [units.get(k) for k in ("prefix", "unit", "base_dimensions", "derived_dimensions")]
    text = " ".join(p for p in parts if p)
    if units.get("system"):
        text += f" ({units['system']})"
    return text


def _number(value):
    return int(value) if isinstance(value, float) and value.is_integer() else value


def _dccl_field(dccl):
    parts = []
    for key, value in dccl.items():
        if key == "units":
            parts.append(f"units {_units(value)}")
        elif key == "description":
            continue
        elif value is True:
            parts.append(key)
        else:
            parts.append(f"{key} {_number(value)}")
    return ", ".join(parts)


def _type_name(field):
    if field.type_name:
        return field.type_name.lstrip(".")
    return descriptor_pb2.FieldDescriptorProto.Type.Name(field.type)[len("TYPE_"):].lower()


def _message_summary(dccl):
    parts = []
    if "id" in dccl:
        parts.append(f"DCCL ID {dccl['id']}")
    if "max_bytes" in dccl:
        parts.append(f"at most {dccl['max_bytes']} bytes")
    if "codec_version" in dccl:
        parts.append(f"codec version {dccl['codec_version']}")
    if "unit_system" in dccl:
        parts.append(f"unit system {dccl['unit_system']}")
    return " · ".join(parts)


class _Renderer:
    def __init__(self, options, file):
        self.options = options
        self.file = file
        self.comments = _comments(file)
        self.lines = []
        self.dccl_ids = []

    def message(self, message, path, scope, level):
        full = f"{scope}.{message.name}"
        self.lines += [f"{'#' * level} `{full}`", ""]
        dccl = self.options.message(message, "dccl.msg")
        if dccl:
            self.lines += [_message_summary(dccl), ""]
            if "id" in dccl:
                self.dccl_ids.append((int(dccl["id"]), full, dccl.get("max_bytes", "")))
        comment = self.comments.get(tuple(path))
        if comment:
            self.lines += [comment, ""]
        if message.field:
            self.lines += ["| Field | Type | DCCL | Description |", "|---|---|---|---|"]
            for i, field in enumerate(message.field):
                description = [self.comments.get(tuple(path + [_FIELD, i]), "")]
                for key in ("goby", "lamss"):
                    extra = self.options.field(field, _FIELD_EXTENSIONS[key])
                    if extra.get("description"):
                        description.append(extra["description"])
                    if key == "lamss" and extra.get("units"):
                        description.append(f"units: {extra['units']}")
                dccl_field = self.options.field(field, _FIELD_EXTENSIONS["dccl"])
                default = f" = {field.default_value}" if field.HasField("default_value") else ""
                self.lines.append(
                    f"| `{field.name}` ({field.number}) "
                    f"| {_LABEL.get(field.label, '')} {_cell(_type_name(field))}{_cell(default)} "
                    f"| {_cell(_dccl_field(dccl_field))} "
                    f"| {_cell(' '.join(d for d in description if d))} |")
            self.lines.append("")
        for i, enum in enumerate(message.enum_type):
            self.enum(enum, path + [_NESTED_ENUM, i], full, level + 1)
        for i, nested in enumerate(message.nested_type):
            if nested.options.map_entry:
                continue
            self.message(nested, path + [_NESTED, i], full, level + 1)

    def enum(self, enum, path, scope, level):
        self.lines += [f"{'#' * level} `{scope}.{enum.name}` (enum)", ""]
        comment = self.comments.get(tuple(path))
        if comment:
            self.lines += [comment, ""]
        self.lines += ["| Value | Number | Description |", "|---|---|---|"]
        for i, value in enumerate(enum.value):
            text = self.comments.get(tuple(path + [_VALUE, i]), "")
            self.lines.append(f"| `{value.name}` | {value.number} | {_cell(text)} |")
        self.lines.append("")

    def render(self):
        for i, message in enumerate(self.file.message_type):
            self.message(message, [_MESSAGE, i], self.file.package, 3)
        for i, enum in enumerate(self.file.enum_type):
            self.enum(enum, [_ENUM, i], self.file.package, 3)
        return self.lines


def render_toolbox(descriptor_set, root, toolbox):
    """(markdown, [(dccl id, message, max bytes)]) for one toolbox's messages page.

    """
    _, names = library_protos(root, toolbox)
    names = set(names)
    options = Options(descriptor_set)
    lines = [f"# {toolbox} messages", "",
             f"Generated from the `.proto` files in `{toolbox}/{PROTO_DIR}`. Comments in the "
             "`.proto` files and the `description` options become the descriptions below.", ""]
    ids = []
    files = [f for f in descriptor_set.file if f.name in names]
    if not files:
        lines.append(f"{toolbox} has no protobuf library.")
    for file in sorted(files, key=lambda f: f.name):
        lines += [f"## [{file.name}](repo:{toolbox}/{PROTO_DIR}/{file.name})", ""]
        renderer = _Renderer(options, file)
        lines += renderer.render()
        ids += [(i, m, b, toolbox) for i, m, b in renderer.dccl_ids]
    return "\n".join(lines) + "\n", ids


def render_index(ids, toolboxes):
    lines = ["# Messages", "",
             "Every DCCL message in the active toolboxes, by ID. IDs are allocated in "
             "[DCCL IDs](../dccl-ids.md).", "",
             "| DCCL ID | Message | Max bytes | Toolbox |", "|---|---|---|---|"]
    for dccl_id, message, max_bytes, toolbox in sorted(ids):
        anchor = message.replace(".", "").lower()
        lines.append(f"| {dccl_id} | [`{message}`]({toolbox}.md#{anchor}) | {max_bytes} | "
                     f"{toolbox} |")
    lines += ["", "Pages per toolbox:", ""]
    lines += [f"- [{tb}]({tb}.md)" for tb in toolboxes]
    return "\n".join(lines) + "\n"
