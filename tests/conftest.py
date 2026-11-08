"""Shared pytest configuration and fixtures for the perf_regions tests."""

from __future__ import annotations

import json
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

import pytest


DEFAULT_PAPI_COUNTERS = "PAPI_TOT_CYC,WALLCLOCKTIME"


def pytest_addoption(parser: pytest.Parser) -> None:
    parser.addoption(
        "--perf-regions-papi-hardware",
        action="store_true",
        default=False,
        help="Run tests with real PAPI hardware counters.",
    )
    parser.addoption(
        "--perf-regions-counters",
        default=None,
        help="PERF_REGIONS_COUNTERS value for PAPI hardware tests.",
    )


@pytest.fixture(scope="session")
def repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


@pytest.fixture(scope="session")
def tests_root() -> Path:
    return Path(__file__).resolve().parent


@pytest.fixture(scope="session")
def build_dir(repo_root: Path) -> Path:
    return repo_root / "build"


@pytest.fixture(scope="session")
def instrument_script(tests_root: Path) -> Path:
    return tests_root / "tools" / "instrument.py"


@pytest.fixture
def base_dir(request: pytest.FixtureRequest) -> Path:
    return Path(request.module.__file__).resolve().parent


@pytest.fixture(scope="session")
def papi_hardware_available(request: pytest.FixtureRequest) -> bool:
    return bool(
        request.config.getoption("--perf-regions-papi-hardware")
        or os.environ.get("PERF_REGIONS_TEST_PAPI_HARDWARE") == "1"
    )


@pytest.fixture(scope="session")
def papi_counters(request: pytest.FixtureRequest) -> str:
    return (
        request.config.getoption("--perf-regions-counters")
        or os.environ.get("PERF_REGIONS_COUNTERS")
        or DEFAULT_PAPI_COUNTERS
    )


@pytest.fixture
def output_json(tmp_path: Path) -> Path:
    return tmp_path / "output_perf_regions.json"


@pytest.fixture
def env(
    request: pytest.FixtureRequest,
    build_dir: Path,
    output_json: Path,
    papi_hardware_available: bool,
    papi_counters: str,
) -> dict[str, str]:
    env_vars = os.environ.copy()

    ld_library_path = env_vars.get("LD_LIBRARY_PATH", "")
    build_path = str(build_dir)
    if build_path not in ld_library_path.split(os.pathsep):
        env_vars["LD_LIBRARY_PATH"] = (
            f"{build_path}{os.pathsep}{ld_library_path}"
            if ld_library_path
            else build_path
        )

    env_vars["PERF_REGIONS_OUTPUT"] = f"json={output_json}"

    mode = getattr(request, "param", "default")
    if mode == "wallclock" or not papi_hardware_available:
        env_vars["PERF_REGIONS_COUNTERS"] = "WALLCLOCKTIME"
    else:
        env_vars["PERF_REGIONS_COUNTERS"] = papi_counters

    return env_vars


@pytest.fixture
def run_command():
    def _run(
        cmd,
        cwd: Path | str | None = None,
        env: dict[str, str] | None = None,
        check: bool = True,
        capture_output: bool = False,
    ) -> subprocess.CompletedProcess:
        if isinstance(cmd, str):
            cmd = shlex.split(cmd)
        cmd = [str(part) for part in cmd]
        return subprocess.run(
            cmd,
            cwd=cwd,
            env=env,
            check=check,
            text=True,
            stdout=subprocess.PIPE if capture_output else None,
            stderr=subprocess.STDOUT if capture_output else None,
        )

    return _run


@pytest.fixture(scope="session")
def cpu_affinity_prefix() -> list[str]:
    if shutil.which("taskset") is None:
        return []

    try:
        subprocess.run(
            ["taskset", "-c", "0", "true"],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
    except (OSError, subprocess.CalledProcessError):
        return []

    return ["taskset", "-c", "0"]


@pytest.fixture(autouse=True)
def clean_artifact(base_dir: Path, env: dict[str, str], run_command) -> None:
    """Start each test from a clean test-directory build state."""

    run_command(["make", "clean"], cwd=base_dir, env=env)

    for path in base_dir.glob("output_perf_regions*.json"):
        path.unlink(missing_ok=True)

    yield

    run_command(["make", "clean"], cwd=base_dir, env=env)

    for path in base_dir.glob("output_perf_regions*.json"):
        path.unlink(missing_ok=True)


@pytest.fixture
def read_json():
    def _read(path: Path):
        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle)

    return _read
