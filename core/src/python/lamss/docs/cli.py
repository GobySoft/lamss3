# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""`lamss docs`: build, serve and deploy the documentation site.

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
    """Runs a console script of this interpreter's environment (mkdocs, mike)."""
    bin_dir = Path(sys.executable).parent
    script = bin_dir / command[0]
    # mike runs mkdocs from PATH
    env = dict(os.environ, PATH=f"{bin_dir}{os.pathsep}{os.environ.get('PATH', '')}")
    return subprocess.run([str(script) if script.exists() else command[0], *command[1:]],
                          env=env).returncode


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


def cmd_deploy(root, args):
    _, config = _stage(root)
    command = ["mike", "deploy", "-F", str(config), "--update-aliases", args.version, *args.alias]
    if args.push:
        command.append("--push")
    status = _run(command)
    if status == 0 and args.set_default:
        status = _run(["mike", "set-default", "-F", str(config), args.set_default]
                      + (["--push"] if args.push else []))
    return status


def _page_for(root, name):
    path = Path(name)
    if path.suffix == ".md":
        return path if path.is_absolute() else Path.cwd() / path
    matches = sorted(root.glob(f"*/src/doc/*/{name}.md"))
    if len(matches) != 1:
        raise SystemExit(f"lamss docs: {len(matches)} pages named {name}.md; give the path")
    return matches[0]


def _new_page(root, source, by):
    from .extract import new_page

    page, text = new_page(root, source, by)
    if page.exists():
        raise SystemExit(f"lamss docs: {page.relative_to(root)} exists; extract without --new")
    page.parent.mkdir(parents=True, exist_ok=True)
    page.write_text(text)
    return page


def cmd_extract(root, args):
    from .extract import update_page

    if args.new:
        page = _new_page(root, Path(args.target), args.by)
        print(f"Created {page.relative_to(root)} from the template; fill in the prose")
    else:
        page = _page_for(root, args.target)
    text = update_page(root, page)
    if args.stdout:
        sys.stdout.write(text)
    elif text != page.read_text():
        page.write_text(text)
        print(f"Updated {page.relative_to(root)}")
    return 0


def cmd_check(root, args):
    import difflib

    from .extract import generated_pages, update_page

    stale = 0
    for page in generated_pages(root):
        current = page.read_text()
        expected = update_page(root, page)
        if expected != current:
            stale += 1
            rel = str(page.relative_to(root))
            sys.stdout.writelines(difflib.unified_diff(
                current.splitlines(keepends=True), expected.splitlines(keepends=True),
                f"a/{rel}", f"b/{rel}"))
    if stale:
        print(f"\n{stale} page(s) do not match the code: run `lamss docs extract <page>`",
              file=sys.stderr)
        return 1
    print("Every page's interface tables match the code")
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

    deploy = sub.add_parser("deploy", help="Publish a version to the gh-pages branch with mike")
    deploy.add_argument("version", help="Version to publish, e.g. dev or 1.0")
    deploy.add_argument("alias", nargs="*", help="Aliases to point at it, e.g. latest")
    deploy.add_argument("--push", action="store_true", help="Push gh-pages to origin")
    deploy.add_argument("--set-default", metavar="VERSION",
                        help="Version or alias the site's root redirects to")
    deploy.set_defaults(func=cmd_deploy)

    extract = sub.add_parser("extract",
                             help="Fill an inferred page's interface tables from the code")
    extract.add_argument("target", help="Page (path or app name), or with --new the source "
                                        "directory, e.g. core/src/bin/pViewMessage")
    extract.add_argument("--new", action="store_true",
                         help="Create the page from the template first")
    extract.add_argument("--stdout", action="store_true", help="Print instead of writing")
    extract.add_argument("--by", default="<agent>",
                         help="With --new: who infers the page (inferred_by), e.g. Claude")
    extract.set_defaults(func=cmd_extract)

    check = sub.add_parser("check", help="Fail if any page's interface tables differ from the code")
    check.set_defaults(func=cmd_check)
    return p


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    args = parser().parse_args(argv)
    root = find_root().resolve()
    if not _in_venv(root) and os.environ.get("LAMSS_DOCS_NO_VENV") != "1":
        python = ensure_venv(root)
        os.execv(str(python), [str(python), "-m", "lamss.docs", *argv])
    return args.func(root, args)
