# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Plan 10's reproducibility check: generating the mbat cruise (and its smoke variant) twice gives
byte-identical output, so no timestamps or unordered dicts reach the configurations."""

import shutil
from pathlib import Path

import pytest

from lamss.mission.cli import main

ROOT = Path(__file__).resolve().parents[4]
INSTANCE = "mbat_generate_check"


@pytest.mark.parametrize("variant", [[], ["smoke"]], ids=["mbat", "smoke"])
def test_generating_mbat_twice_is_byte_identical(monkeypatch, variant):
    monkeypatch.setenv("LAMSS_ROOT", str(ROOT))
    build = ROOT / "mit" / "missions" / "build" / INSTANCE
    try:
        assert main(["generate", "mbat", *variant, "--instance", INSTANCE]) == 0
        first = {p: p.read_bytes() for p in build.rglob("*") if p.is_file()}
        assert main(["generate", "mbat", *variant, "--instance", INSTANCE]) == 0
        second = {p: p.read_bytes() for p in build.rglob("*") if p.is_file()}
        assert first and first == second
    finally:
        shutil.rmtree(build, ignore_errors=True)
