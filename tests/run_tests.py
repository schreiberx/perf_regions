#!/usr/bin/env python3
"""Canonical test runner for perf_regions.

Examples:
    python3 tests/run_tests.py
    python3 tests/run_tests.py --mpi --papi local
    python3 tests/run_tests.py --cc clang --no-mpi --papi none
    python3 tests/run_tests.py --papi local --mpi --papi-hardware
"""

from __future__ import annotations

import argparse
import importlib.util
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
TESTS_DIR = REPO_ROOT / "tests"
DEFAULT_PAPI_COUNTERS = "PAPI_TOT_CYC,WALLCLOCKTIME"
LOCAL_PAPI_INSTALLER = TESTS_DIR / "tools" / "papi-local" / "install.sh"


def run(
    cmd: list[str | Path],
    *,
    cwd: Path = REPO_ROOT,
    env: dict[str, str] | None = None,
    check: bool = True,
) -> subprocess.CompletedProcess:
    rendered = [str(part) for part in cmd]
    print("+ " + " ".join(shlex.quote(part) for part in rendered), flush=True)
    return subprocess.run(rendered, cwd=cwd, env=env, check=check, text=True)


def prepend_paths(env: dict[str, str], name: str, paths: list[Path]) -> None:
    current = env.get(name, "")
    entries = [entry for entry in current.split(os.pathsep) if entry]

    for path in reversed(paths):
        value = str(path)
        if value and value not in entries:
            entries.insert(0, value)

    env[name] = os.pathsep.join(entries)


def detect_libdir(prefix: Path) -> str:
    for libdir in ("lib64", "lib"):
        candidate = prefix / libdir
        if (candidate / "libpapi.so").exists() or (candidate / "libpapi.a").exists():
            return libdir
    return "lib"


def find_local_papi_prefix() -> Path | None:
    candidates = [
        os.environ.get("PERF_REGIONS_TEST_PAPI_PREFIX"),
        os.environ.get("PAPI_PREFIX"),
        str(TESTS_DIR / ".deps" / "papi" / "current"),
    ]

    for candidate in candidates:
        if not candidate:
            continue
        prefix = Path(candidate).expanduser()
        if (prefix / ".perf_regions_papi_install").is_file():
            return prefix.resolve()

    return None


def install_local_papi(args: argparse.Namespace, env: dict[str, str]) -> Path:
    if args.papi_prefix:
        prefix = Path(args.papi_prefix).expanduser().resolve()
    else:
        prefix = find_local_papi_prefix()
        if prefix is None:
            run(
                [LOCAL_PAPI_INSTALLER, "--jobs", str(args.jobs)],
                cwd=REPO_ROOT,
                env=env,
            )
            prefix = (TESTS_DIR / ".deps" / "papi" / "current").resolve()

    if not (prefix / "include" / "papi.h").is_file():
        raise SystemExit(f"ERROR: local PAPI prefix has no include/papi.h: {prefix}")

    return prefix


def configure_local_papi(env: dict[str, str], prefix: Path) -> None:
    libdir = detect_libdir(prefix)
    env["PAPI_PREFIX"] = str(prefix)
    env["PERF_REGIONS_TEST_PAPI_PREFIX"] = str(prefix)
    env["PAPI_CFLAGS"] = f"-I{prefix / 'include'}"
    env["PAPI_LDFLAGS"] = f"-L{prefix / libdir} -lpapi"

    prepend_paths(env, "PATH", [prefix / "bin"])
    prepend_paths(env, "LD_LIBRARY_PATH", [prefix / "lib", prefix / "lib64"])
    prepend_paths(
        env,
        "PKG_CONFIG_PATH",
        [prefix / "lib" / "pkgconfig", prefix / "lib64" / "pkgconfig"],
    )


def prepare_python(args: argparse.Namespace) -> Path:
    if args.venv:
        venv_dir = TESTS_DIR / ".venv"
        python = venv_dir / "bin" / "python"

        usable = python.exists() and subprocess.run(
            [str(python), "-m", "pip", "--version"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        ).returncode == 0

        if not usable:
            if venv_dir.exists():
                print(f"Recreating unusable virtual environment in {venv_dir}")
                shutil.rmtree(venv_dir)
            else:
                print(f"Creating virtual environment in {venv_dir}")
            run([sys.executable, "-m", "venv", venv_dir])

        run(
            [
                python,
                "-m",
                "pip",
                "install",
                "-q",
                "--disable-pip-version-check",
                "-r",
                TESTS_DIR / "requirements.txt",
            ]
        )
        return python

    if importlib.util.find_spec("pytest") is None:
        raise SystemExit(
            "ERROR: pytest is not installed in this Python environment.\n"
            "Install the test dependencies with:\n"
            f"    {sys.executable} -m pip install -r {TESTS_DIR / 'requirements.txt'}\n"
            "or rerun with --venv."
        )

    return Path(sys.executable)


def build_environment(args: argparse.Namespace) -> tuple[dict[str, str], Path | None]:
    env = os.environ.copy()
    env["CC"] = args.cc
    env["CXX"] = args.cxx
    env["FC"] = args.fc

    papi_prefix: Path | None = None
    if args.papi == "local":
        papi_prefix = install_local_papi(args, env)
        configure_local_papi(env, papi_prefix)
    elif args.papi == "system" and args.papi_prefix:
        papi_prefix = Path(args.papi_prefix).expanduser().resolve()
        configure_local_papi(env, papi_prefix)

    return env, papi_prefix


def configure(args: argparse.Namespace, env: dict[str, str], papi_prefix: Path | None) -> None:
    cmd: list[str | Path] = [
        REPO_ROOT / "configure",
        "--enable-fortran" if args.fortran else "--disable-fortran",
        "--enable-mpi" if args.mpi else "--disable-mpi",
    ]

    if args.papi == "none":
        cmd.append("--disable-papi")
    elif args.papi == "local":
        if papi_prefix is None:
            raise SystemExit("ERROR: --papi local requires a local PAPI prefix")
        cmd.extend(["--enable-papi", f"--with-papi={papi_prefix}"])
    else:
        if papi_prefix is not None:
            cmd.extend(["--enable-papi", f"--with-papi={papi_prefix}"])
        else:
            cmd.append("--enable-papi")

    run(cmd, cwd=REPO_ROOT, env=env)


def build(args: argparse.Namespace, env: dict[str, str]) -> None:
    run(["make", "-j", str(args.jobs), "USE_FORTRAN=0"], cwd=REPO_ROOT, env=env)
    if args.fortran:
        run(["make", "-j", str(args.jobs), "USE_FORTRAN=1"], cwd=REPO_ROOT, env=env)


def pytest_command(args: argparse.Namespace, python: Path) -> list[str | Path]:
    cmd: list[str | Path] = [python, "-m", "pytest", "-q"]

    if args.fast:
        cmd.extend(["-m", "not slow"])

    if not args.fortran:
        cmd.extend(
            [
                "--ignore=array_test_f",
                "--ignore=nemo_test_f",
                "--ignore=nemo_test_f_direct",
            ]
        )

    if args.papi_hardware:
        cmd.append("--perf-regions-papi-hardware")
        if args.counters:
            cmd.extend(["--perf-regions-counters", args.counters])

    if args.pytest_args:
        cmd.extend(shlex.split(args.pytest_args))

    cmd.extend(args.pytest_target or [])

    return cmd


def prepare_test_environment(
    args: argparse.Namespace, env: dict[str, str]
) -> dict[str, str]:
    test_env = env.copy()

    prepend_paths(test_env, "LD_LIBRARY_PATH", [REPO_ROOT / "build"])

    test_env["USE_MPI"] = "1" if args.mpi else "0"
    test_env["USE_PAPI"] = "0" if args.papi == "none" else "1"

    if args.papi_hardware:
        counters = args.counters or os.environ.get("PERF_REGIONS_COUNTERS") or DEFAULT_PAPI_COUNTERS
        test_env["PERF_REGIONS_COUNTERS"] = counters
        test_env["PERF_REGIONS_TEST_PAPI_HARDWARE"] = "1"
    else:
        test_env["PERF_REGIONS_COUNTERS"] = "WALLCLOCKTIME"
        test_env.pop("PERF_REGIONS_TEST_PAPI_HARDWARE", None)

    return test_env


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Configure, build, and run the perf_regions test suite."
    )

    parser.add_argument("--cc", default=os.environ.get("CC", "gcc"))
    parser.add_argument("--cxx", default=os.environ.get("CXX", "g++"))
    parser.add_argument("--fc", default=os.environ.get("FC", "gfortran"))
    parser.add_argument("--jobs", default=str(os.cpu_count() or 1))

    parser.add_argument("--mpi", action="store_true", help="Build with MPI.")
    parser.add_argument(
        "--no-mpi", action="store_false", dest="mpi", help="Build without MPI."
    )
    parser.set_defaults(mpi=False)

    parser.add_argument(
        "--papi",
        choices=("none", "system", "local"),
        default="none",
        help="PAPI mode: no PAPI, system PAPI, or the test-local installation.",
    )
    parser.add_argument(
        "--papi-prefix",
        default=None,
        help="Explicit PAPI prefix for --papi local/system.",
    )

    parser.add_argument(
        "--fortran",
        action="store_true",
        dest="fortran",
        help="Build/test Fortran (default).",
    )
    parser.add_argument(
        "--no-fortran",
        action="store_false",
        dest="fortran",
        help="Skip Fortran build and tests.",
    )
    parser.set_defaults(fortran=True)

    parser.add_argument(
        "--papi-hardware",
        action="store_true",
        help="Run PAPI-counter tests instead of wallclock-only tests.",
    )
    parser.add_argument(
        "--wallclock-only",
        action="store_true",
        help="Explicitly request wallclock-only tests (the default).",
    )
    parser.add_argument(
        "--counters",
        default=None,
        help=f"Counter list for --papi-hardware (default: {DEFAULT_PAPI_COUNTERS}).",
    )

    parser.add_argument(
        "--venv",
        action="store_true",
        help="Create/use tests/.venv and install test requirements.",
    )
    parser.add_argument(
        "--fast",
        action="store_true",
        help="Exclude tests marked as slow.",
    )
    parser.add_argument(
        "--pytest-args",
        default="",
        help="Additional arguments passed to pytest.",
    )
    parser.add_argument(
        "--pytest-target",
        action="append",
        default=[],
        help="Pytest target (directory or file). May be repeated.",
    )

    parser.add_argument(
        "--no-configure",
        action="store_true",
        help="Do not run ./configure.",
    )
    parser.add_argument(
        "--no-build",
        action="store_true",
        help="Do not build the library (reuse an existing build).",
    )
    parser.add_argument(
        "--keep-build",
        action="store_true",
        help="Do not clean before/after the run.",
    )

    args = parser.parse_args(argv)

    if args.papi_hardware and args.wallclock_only:
        parser.error("--papi-hardware and --wallclock-only are mutually exclusive")
    if args.papi_hardware and args.papi == "none":
        parser.error("--papi-hardware requires --papi system or --papi local")

    return args


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    if args.fortran and shutil.which(args.fc) is None:
        print(
            f"WARNING: Fortran compiler '{args.fc}' not found; "
            "skipping the Fortran build and tests.",
            flush=True,
        )
        args.fortran = False

    python = prepare_python(args)
    env, papi_prefix = build_environment(args)

    if not args.keep_build and not args.no_build:
        run(["make", "clean"], cwd=REPO_ROOT, env=env, check=False)

    try:
        if not args.no_configure:
            configure(args, env, papi_prefix)

        if not args.no_build:
            build(args, env)

        stale_config = TESTS_DIR / "config_tests.mk"
        stale_config.unlink(missing_ok=True)

        test_env = prepare_test_environment(args, env)

        if args.papi_hardware and args.papi == "local":
            doctor = TESTS_DIR / "tools" / "papi-local" / "doctor.sh"
            result = subprocess.run(
                [str(doctor), "--strict"],
                cwd=REPO_ROOT,
                env=test_env,
                text=True,
                check=False,
            )
            if result.returncode != 0:
                print(
                    "PAPI hardware counters are unavailable; "
                    "skipping the hardware-counter test run.",
                    flush=True,
                )
                return 0

        run(pytest_command(args, python), cwd=TESTS_DIR, env=test_env)

        return 0
    finally:
        if not args.keep_build and not args.no_build:
            run(["make", "clean"], cwd=REPO_ROOT, env=env, check=False)


if __name__ == "__main__":
    raise SystemExit(main())
