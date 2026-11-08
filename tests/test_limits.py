"""Consistency checks for limits shared between C and the instrumenter."""

from __future__ import annotations

import re
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


def _header_define(name: str) -> int:
    text = (REPO_ROOT / "src" / "perf_regions_defines.h").read_text()
    match = re.search(rf"^#define\s+{re.escape(name)}\s+\((\d+)\)", text, re.MULTILINE)
    assert match is not None, f"{name} not found in perf_regions_defines.h"
    return int(match.group(1))


def _python_constant(name: str) -> int:
    text = (REPO_ROOT / "scripts" / "perf_regions.py").read_text()
    match = re.search(rf"^{re.escape(name)}\s*=\s*(\d+)", text, re.MULTILINE)
    assert match is not None, f"{name} not found in perf_regions.py"
    return int(match.group(1))


def test_region_limit_matches_instrumenter() -> None:
    assert _python_constant("PERF_REGIONS_MAX") == _header_define("PERF_REGIONS_MAX")
