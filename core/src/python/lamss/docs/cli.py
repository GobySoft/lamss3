# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""`lamss docs`: build and serve the documentation site.

mkdocs and its plugins run from a virtual environment in build/docs-venv, made from
docs/requirements.txt on first use, because Ubuntu's packages lag and miss plugins.
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path


def find_root():
    if os.environ.get("LAMSS_ROOT"):
        return Path(os.environ["LAMSS_ROOT"])
    here = Path.cwd().resolve()
    for candidate in [here, *here.parents]:
        if (candidate / ".lamss").is_dir() and (candidate / "docs" / "nav.yml").is_file():
            return candidate
    raise SystemExit("lamss docs: not inside a lamss3 checkout")


def ensure_venv(root):
    venv = root / "build" / "docs-venv"
    python = venv / "bin" / "python3"
    requirements = root / "docs" / "requirements.txt"
    stamp = venv / "lamss-requirements.txt"
    if not python.exists():
        print(f"Creating {venv} for mkdocs", file=sys.stderr)
        if subprocess.run([sys.executable, "-m", "venv", str(venv)]).returncode != 0:
            raise SystemExit("lamss docs: cannot create a virtual environment "
                             "(apt install python3-venv)")
    if not stamp.is_file() or stamp.read_text() != requirements.read_text():
        print(f"Installing {requirements.relative_to(root)} into {venv}", file=sys.stderr)
        command = [str(python), "-m", "pip", "install", "--quiet", "-r", str(requirements)]
        if subprocess.run(command).returncode != 0:
            raise SystemExit("lamss docs: pip install failed")
        stamp.write_text(requirements.read_text())
    return python


def _in_venv(root):
    return Path(sys.prefix).resolve() == (root / "build" / "docs-venv").resolve()


def _paths(root):
    return root / "build" / "docs" / "stage", root / "build" / "docs" / "site"


def _stage(root, serve=False):
    # imported here: the system Python runs this module before it re-executes in the venv
    from .stage import Stage

    stage_dir, site_dir = _paths(root)
    stage = Stage(root, stage_dir)
    config = stage.run(site_dir, serve)
    for warning in stage.warnings:
        print(f"WARNING  -  lamss docs: {warning}", file=sys.stderr)
    return stage, config


def _run(command):
    """Runs a console script of this interpreter's environment (mkdocs)."""
    script = Path(sys.executable).parent / command[0]
    return subprocess.run([str(script) if script.exists() else command[0], *command[1:]]).returncode


def cmd_build(root, args):
    stage, config = _stage(root)
    if args.strict and stage.warnings:
        print("Aborted with warnings in strict mode", file=sys.stderr)
        return 1
    status = _run(["mkdocs", "build", "-f", str(config)] + (["--strict"] if args.strict else []))
    if status == 0:
        print(f"Site: {_paths(root)[1] / 'index.html'}")
    return status


def cmd_serve(root, args):
    _, config = _stage(root, serve=True)
    os.environ["LAMSS_DOCS_RESTAGE"] = "1"
    return _run(["mkdocs", "serve", "-f", str(config), "--dev-addr", args.dev_addr])


def cmd_stage(root, args):
    _, config = _stage(root)
    print(config)
    return 0


def parser():
    p = argparse.ArgumentParser(prog="lamss docs", description=__doc__.splitlines()[0])
    sub = p.add_subparsers(dest="command", required=True, metavar="command")

    build = sub.add_parser("build", help="Build the site into build/docs/site")
    build.add_argument("--strict", action="store_true",
                       help="Fail on any warning (broken links, missing pages), as CI does")
    build.set_defaults(func=cmd_build)

    serve = sub.add_parser("serve", help="Serve the site with live reload while editing pages")
    serve.add_argument("--dev-addr", default="127.0.0.1:8000", help="Address to serve on")
    serve.set_defaults(func=cmd_serve)

    stage = sub.add_parser("stage", help="Only assemble build/docs/stage (for debugging)")
    stage.set_defaults(func=cmd_stage)
    return p


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    args = parser().parse_args(argv)
    root = find_root().resolve()
    if not _in_venv(root) and os.environ.get("LAMSS_DOCS_NO_VENV") != "1":
        python = ensure_venv(root)
        os.execv(str(python), [str(python), "-m", "lamss.docs", *argv])
    return args.func(root, args)
