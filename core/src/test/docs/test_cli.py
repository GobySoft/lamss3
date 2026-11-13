# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

import os
import subprocess
import sys
from pathlib import Path


def test_cli_imports_without_site_packages():
    """`lamss docs` starts under the system Python, which may lack mkdocs, protobuf and PyYAML,
    and only then re-executes in its virtual environment."""
    python = Path(__file__).resolve().parents[2] / "python"
    env = dict(os.environ, PYTHONPATH=str(python))
    result = subprocess.run([sys.executable, "-S", "-c", "import lamss.docs.cli"], env=env,
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
