#!/usr/bin/env python3
"""Shared instrumentation entry point for the perf_regions tests."""

from __future__ import annotations

import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO_ROOT / "scripts"))

import perf_regions  # noqa: E402


if __name__ == "__main__":
    raise SystemExit(perf_regions.main())
