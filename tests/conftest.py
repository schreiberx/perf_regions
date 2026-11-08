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


def config_make_value(repo_root: Path, name: str) -> str | None:
    config = repo_root / "config.mk"
    try:
        lines = config.read_text(encoding="utf-8").splitlines()
    except OSError:
        return None

    prefix = f"{name}="
    for line in lines:
        if line.startswith(prefix):
            return line[len(prefix):].strip()

    return None


def default_build_directory(repo_root: Path) -> Path:
    use_papi = os.environ.get("USE_PAPI")
    if use_papi is None:
        use_papi = config_make_value(repo_root, "USE_PAPI")

    use_mpi = os.environ.get("USE_MPI")
    if use_mpi is None:
        use_mpi = config_make_value(repo_root, "USE_MPI")

    suffix = ""
    if use_papi == "1":
        suffix += "_papi"
    if use_mpi == "1":
        suffix += "_mpi"

    return repo_root / f"build{suffix}"


@pytest.fixture(scope="session")
def build_dir(repo_root: Path) -> Path:
    configured = os.environ.get("PERF_REGIONS_BUILD_DIR")
    if configured:
        return Path(configured)
    return default_build_directory(repo_root)


@pytest.fixture(scope="session")
def instrument_script(tests_root: Path) -> Path:
    return tests_root / "tools" / "instrument.py"


@pytest.fixture
def base_dir(request: pytest.FixtureRequest) -> Path:
    return Path(request.module.__file__).resolve().parent


def papi_hardware_requested(request: pytest.FixtureRequest) -> bool:
    return bool(
        request.config.getoption("--perf-regions-papi-hardware")
        or os.environ.get("PERF_REGIONS_TEST_PAPI_HARDWARE") == "1"
    )


def papi_build_enabled(build_dir: Path) -> bool:
    env_value = os.environ.get("USE_PAPI")
    if env_value == "0":
        return False
    if env_value == "1":
        return True

    config_header = build_dir / "perf_regions_config.h"
    try:
        text = config_header.read_text(encoding="utf-8")
    except OSError:
        return False

    for line in text.splitlines():
        if line.strip() == "#define PERF_REGIONS_USE_PAPI 1":
            return True

    return False


@pytest.fixture(scope="session")
def papi_hardware_available(request: pytest.FixtureRequest) -> bool:
    return papi_hardware_requested(request)


@pytest.fixture(scope="session")
def papi_test_skip_reason(
    request: pytest.FixtureRequest,
    build_dir: Path,
) -> str | None:
    hardware_requested = papi_hardware_requested(request)
    build_enabled = papi_build_enabled(build_dir)

    if not hardware_requested:
        if not build_enabled:
            return (
                "PAPI hardware tests were not run because this build has PAPI "
                "support disabled (USE_PAPI=0). Re-run with --papi local "
                "(or --papi system) and --test-papi-counters."
            )
        return (
            "PAPI hardware tests were not requested; this run uses "
            "wallclock-only tests. Re-run with --test-papi-counters."
        )

    if not build_enabled:
        return (
            "PAPI hardware tests were requested, but this library was built "
            "without PAPI support (USE_PAPI=0). Reconfigure with --papi local "
            "or --papi system and rebuild."
        )

    return None


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

    build_paths = [str(build_dir)]
    fortran_build_path = env_vars.get("PERF_REGIONS_FORTRAN_BUILD_DIR")
    if fortran_build_path:
        build_paths.append(fortran_build_path)

    ld_library_path = env_vars.get("LD_LIBRARY_PATH", "")
    entries = [entry for entry in ld_library_path.split(os.pathsep) if entry]
    for build_path in reversed(build_paths):
        if build_path and build_path not in entries:
            entries.insert(0, build_path)
    env_vars["LD_LIBRARY_PATH"] = os.pathsep.join(entries)

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
