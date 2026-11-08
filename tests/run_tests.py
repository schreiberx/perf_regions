#!/usr/bin/env python3
"""Canonical test runner for perf_regions.

Examples:
    python3 tests/run_tests.py
    python3 tests/run_tests.py --mpi --papi local
    python3 tests/run_tests.py --cc clang --no-mpi --papi none
    python3 tests/run_tests.py --papi local --mpi --test-papi-counters
"""

from __future__ import annotations

import argparse
import copy
import importlib.util
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
TESTS_DIR = REPO_ROOT / "tests"
DEFAULT_PAPI_COUNTERS = "PAPI_TOT_CYC,WALLCLOCKTIME"
LOCAL_PAPI_INSTALLER = TESTS_DIR / "tools" / "papi-local" / "install.sh"
YELLOW = "\033[1;33m"
GREEN = "\033[1;32m"
RED = "\033[1;31m"
CYAN = "\033[1;36m"
BOLD = "\033[1m"
RESET = "\033[0m"


def run(
    cmd: list[str | Path],
    *,
    cwd: Path = REPO_ROOT,
    env: dict[str, str] | None = None,
    check: bool = True,
    quiet: bool = False,
) -> subprocess.CompletedProcess:
    rendered = [str(part) for part in cmd]
    rendered_command = "+ " + " ".join(shlex.quote(part) for part in rendered)

    if not quiet:
        print(rendered_command, flush=True)
        return subprocess.run(rendered, cwd=cwd, env=env, check=check, text=True)

    result = subprocess.run(
        rendered,
        cwd=cwd,
        env=env,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    if result.returncode != 0:
        print(rendered_command, flush=True)
        if result.stdout:
            print(result.stdout, end="", flush=True)
            if not result.stdout.endswith("\n"):
                print(flush=True)
        if check:
            raise subprocess.CalledProcessError(
                result.returncode, result.args, output=result.stdout
            )

    return result


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
                quiet=not args.verbose_build,
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
            run(
                [sys.executable, "-m", "venv", venv_dir],
                quiet=not args.verbose_build,
            )

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
            ],
            quiet=not args.verbose_build,
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

    run(cmd, cwd=REPO_ROOT, env=env, quiet=not args.verbose_build)


def build_variant_suffix(args: argparse.Namespace, include_fortran: bool) -> str:
    suffix = ""
    if args.papi != "none":
        suffix += "_papi"
    if args.mpi:
        suffix += "_mpi"
    if include_fortran and args.fortran:
        suffix += "_fort"
    return suffix


def build_directories(args: argparse.Namespace) -> list[Path]:
    build_root = REPO_ROOT / "build"
    directories = [build_root / f"build{build_variant_suffix(args, False)}"]
    if args.fortran:
        directories.append(build_root / f"build{build_variant_suffix(args, True)}")
    return directories


def clean_build_artifacts(args: argparse.Namespace, env: dict[str, str]) -> None:
    # Only remove the variants used by this invocation.  Other feature
    # combinations keep their own build directories.
    for path in build_directories(args):
        shutil.rmtree(path, ignore_errors=True)

    for module in (REPO_ROOT / "src").glob("*.mod"):
        module.unlink(missing_ok=True)

    run(
        ["make", "-C", "tests", "clean"],
        cwd=REPO_ROOT,
        env=env,
        check=False,
        quiet=not args.verbose_build,
    )


def build(args: argparse.Namespace, env: dict[str, str]) -> None:
    run(
        ["make", "-j", str(args.jobs), "USE_FORTRAN=0"],
        cwd=REPO_ROOT,
        env=env,
        quiet=not args.verbose_build,
    )
    if args.fortran:
        run(
            ["make", "-j", str(args.jobs), "USE_FORTRAN=1"],
            cwd=REPO_ROOT,
            env=env,
            quiet=not args.verbose_build,
        )


def pytest_command(args: argparse.Namespace, python: Path) -> list[str | Path]:
    cmd: list[str | Path] = [python, "-m", "pytest", "-v"]

    if args.fast:
        cmd.extend(["-m", "not slow"])

    if not args.fortran:
        cmd.extend(
            [
                "--ignore=functional/instrumentation/array_test_f",
                "--ignore=functional/instrumentation/nemo_test_f",
                "--ignore=functional/instrumentation/nemo_test_f_direct",
            ]
        )

    if args.test_papi_counters:
        cmd.append("--perf-regions-papi-hardware")
        cmd.extend(["--perf-regions-counters", test_counters(args)])

    if args.pytest_args:
        cmd.extend(shlex.split(args.pytest_args))

    cmd.extend(args.pytest_target or [])

    return cmd


def test_counters(args: argparse.Namespace) -> str:
    if not args.test_papi_counters:
        return "WALLCLOCKTIME"

    counters = args.counters or os.environ.get("PERF_REGIONS_COUNTERS") or DEFAULT_PAPI_COUNTERS
    papi_events = [
        event.strip()
        for event in counters.split(",")
        if event.strip() and event.strip() != "WALLCLOCKTIME"
    ]
    if not papi_events:
        raise SystemExit(
            "ERROR: --test-papi-counters requires at least one non-WALLCLOCKTIME counter"
        )

    if args.test_wallclock:
        papi_events.append("WALLCLOCKTIME")

    return ",".join(papi_events)


def prepare_test_environment(
    args: argparse.Namespace, env: dict[str, str]
) -> dict[str, str]:
    test_env = env.copy()

    build_dirs = build_directories(args)
    prepend_paths(test_env, "LD_LIBRARY_PATH", build_dirs)

    test_env["USE_MPI"] = "1" if args.mpi else "0"
    test_env["USE_PAPI"] = "0" if args.papi == "none" else "1"
    test_env["USE_FORTRAN"] = "1" if args.fortran else "0"
    test_env["PERF_REGIONS_BUILD_DIR"] = str(build_dirs[0])
    if args.fortran:
        test_env["PERF_REGIONS_FORTRAN_BUILD_DIR"] = str(build_dirs[1])
    else:
        test_env.pop("PERF_REGIONS_FORTRAN_BUILD_DIR", None)
    test_env["PERF_REGIONS_COUNTERS"] = test_counters(args)

    if args.test_papi_counters:
        test_env["PERF_REGIONS_TEST_PAPI_HARDWARE"] = "1"
    else:
        test_env.pop("PERF_REGIONS_TEST_PAPI_HARDWARE", None)

    return test_env


def matrix_compile_variants(args: argparse.Namespace) -> list[tuple[bool, str]]:
    if args.matrix == "all":
        mpi_modes = [False]
        if shutil.which("mpicc") and shutil.which("mpif90"):
            mpi_modes = [True, False]
        else:
            print("MPI compilers not found; skipping MPI combinations.", flush=True)

        papi_modes: list[str] = []
        if args.papi != "none":
            papi_modes.append(args.papi)
        papi_modes.append("none")

        return [(mpi, papi) for mpi in mpi_modes for papi in papi_modes]

    if args.matrix == "papi":
        papi_mode = args.papi if args.papi != "none" else "local"
        return [(args.mpi, papi_mode)]

    return [(args.mpi, "none")]


def matrix_runtime_modes(papi_enabled: bool) -> list[tuple[str, bool, bool]]:
    if not papi_enabled:
        return [("Wallclock only", False, True)]

    return [
        ("PAPI counters + wallclock", True, True),
        ("PAPI counters only", True, False),
        ("Wallclock only", False, True),
    ]


def color_text(text: str, color: str) -> str:
    return f"{color}{text}{RESET}" if use_color() else text


def parse_junit(path: Path) -> dict:
    summary = {
        "tests": 0,
        "failures": 0,
        "errors": 0,
        "skipped": 0,
        "details": [],
    }

    if not path.exists():
        return summary

    try:
        root = ET.parse(path).getroot()
    except ET.ParseError:
        return summary

    for testcase in root.iter("testcase"):
        summary["tests"] += 1
        name = "::".join(
            part for part in (testcase.get("classname"), testcase.get("name")) if part
        )
        failure = testcase.find("failure")
        error = testcase.find("error")
        skipped = testcase.find("skipped")

        if failure is not None:
            summary["failures"] += 1
            reason = failure.get("message") or (failure.text or "").strip()
            summary["details"].append(("FAILED", name, reason))
        elif error is not None:
            summary["errors"] += 1
            reason = error.get("message") or (error.text or "").strip()
            summary["details"].append(("ERROR", name, reason))
        elif skipped is not None:
            summary["skipped"] += 1
            reason = skipped.get("message") or (skipped.text or "").strip()
            summary["details"].append(("SKIPPED", name, reason))

    return summary


def local_papi_doctor(args: argparse.Namespace, env: dict[str, str]) -> tuple[bool, str]:
    doctor = TESTS_DIR / "tools" / "papi-local" / "doctor.sh"
    doctor_args = copy.copy(args)
    doctor_args.test_papi_counters = True
    doctor_args.test_wallclock = True
    doctor_env = prepare_test_environment(doctor_args, env)
    doctor_env["PERF_REGIONS_TEST_PAPI_QUIET"] = "1"
    result = subprocess.run(
        [str(doctor), "--strict"],
        cwd=REPO_ROOT,
        env=doctor_env,
        text=True,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return result.returncode == 0, result.stdout or ""


def matrix_result_text(summary: dict) -> str:
    failed = summary["failures"] + summary["errors"]
    return (
        f"{summary['tests']} tests, {failed} failed, "
        f"{summary['skipped']} skipped"
    )


def print_matrix_summary(results: list[dict]) -> int:
    print()
    print(color_text("=" * 64, CYAN))
    print(color_text("  Test matrix summary", BOLD))
    print(color_text("=" * 64, CYAN))
    print(f"{'STATUS':<8}{'COMPILE':<38}RUNTIME")
    print("-" * 64)

    total_tests = 0
    total_failed = 0
    total_skipped = 0
    any_failed = False

    for result in results:
        status = result["status"]
        color = GREEN if status == "PASS" else RED if status == "FAIL" else YELLOW
        compile_label = result["compile"]
        runtime_label = result["runtime"]
        summary = result.get("summary")

        if summary is None:
            details_text = result.get("reason", "")
        else:
            details_text = matrix_result_text(summary)
            total_tests += summary["tests"]
            total_failed += summary["failures"] + summary["errors"]
            total_skipped += summary["skipped"]

        print(color_text(f"{status:<8}", color) + f"{compile_label:<38}{runtime_label}")
        if details_text:
            print(f"        {details_text}")
        if summary is not None:
            for kind, name, reason in summary["details"]:
                print(f"        {kind}: {name}")
                if reason:
                    print(f"          {reason}")

        if status == "FAIL":
            any_failed = True

    print("-" * 64)
    print(
        f"Totals: {total_tests} tests, {total_failed} failed, "
        f"{total_skipped} skipped"
    )
    if any_failed:
        print(color_text("Test matrix completed with failures.", RED))
        return 1

    print(color_text("All test combinations completed successfully.", GREEN))
    return 0


def run_matrix(args: argparse.Namespace, python: Path) -> int:
    results: list[dict] = []
    compile_variants = matrix_compile_variants(args)

    for mpi, papi_mode in compile_variants:
        variant_args = copy.copy(args)
        variant_args.mpi = mpi
        variant_args.papi = papi_mode
        variant_args.matrix = "none"

        mpi_label = "MPI enabled" if mpi else "MPI disabled"
        papi_label = f"PAPI enabled ({papi_mode})" if papi_mode != "none" else "PAPI disabled"
        compile_label = f"{mpi_label}, {papi_label}"

        print()
        print(color_text("=" * 64, CYAN))
        print(color_text(f"  Compile: {compile_label}", BOLD))
        print(color_text("=" * 64, CYAN))

        env, papi_prefix = build_environment(variant_args)

        if not variant_args.keep_build and not variant_args.no_build:
            clean_build_artifacts(variant_args, env)

        try:
            if not variant_args.no_build:
                if not variant_args.no_configure:
                    configure(variant_args, env, papi_prefix)
                build(variant_args, env)
        except subprocess.CalledProcessError as exc:
            results.append(
                {
                    "compile": compile_label,
                    "runtime": "build",
                    "status": "FAIL",
                    "summary": None,
                    "reason": f"Build failed with exit code {exc.returncode}",
                }
            )
            if not variant_args.keep_build:
                clean_build_artifacts(variant_args, env)
            continue

        doctor_ok = True
        doctor_output = ""
        if papi_mode != "none" and papi_mode == "local":
            doctor_ok, doctor_output = local_papi_doctor(variant_args, env)
            if not doctor_ok and doctor_output:
                print(doctor_output, end="", flush=True)
                if not doctor_output.endswith("\n"):
                    print(flush=True)

        for runtime_label, use_papi_counters, use_wallclock in matrix_runtime_modes(
            papi_mode != "none"
        ):
            run_args = copy.copy(variant_args)
            run_args.test_papi_counters = use_papi_counters
            run_args.test_wallclock = use_wallclock

            print()
            print(color_text(f"  Runtime: {runtime_label}", BOLD))

            if use_papi_counters and not doctor_ok:
                status = "FAIL" if run_args.require_papi_hardware else "SKIP"
                results.append(
                    {
                        "compile": compile_label,
                        "runtime": runtime_label,
                        "status": status,
                        "summary": None,
                        "reason": "PAPI hardware counters are unavailable",
                    }
                )
                continue

            test_env = prepare_test_environment(run_args, env)
            temp_dir = Path(tempfile.mkdtemp(prefix="perf-regions-pytest-"))
            junit_path = temp_dir / "results.xml"
            command = pytest_command(run_args, python)
            command.append(f"--junitxml={junit_path}")

            try:
                completed = run(
                    command,
                    cwd=TESTS_DIR,
                    env=test_env,
                    check=False,
                )
                summary = parse_junit(junit_path)
                status = "PASS" if completed.returncode == 0 else "FAIL"
                results.append(
                    {
                        "compile": compile_label,
                        "runtime": runtime_label,
                        "status": status,
                        "summary": summary,
                    }
                )
            finally:
                shutil.rmtree(temp_dir, ignore_errors=True)

        if not variant_args.keep_build:
            clean_build_artifacts(variant_args, env)

    return print_matrix_summary(results)


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
        "--test-papi-counters",
        action="store_true",
        help="Run PAPI hardware-counter tests.",
    )
    parser.add_argument(
        "--test-wallclock",
        action="store_true",
        dest="test_wallclock",
        help="Include wallclock timing in tests (default).",
    )
    parser.add_argument(
        "--test-no-wallclock",
        action="store_false",
        dest="test_wallclock",
        help="Disable wallclock timing in tests.",
    )
    parser.set_defaults(test_wallclock=True)
    parser.add_argument(
        "--counters",
        default=None,
        help=f"Counter list for --test-papi-counters (default: {DEFAULT_PAPI_COUNTERS}).",
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
        "--verbose-build",
        action="store_true",
        help="Show configure, build, and dependency-install output.",
    )
    parser.add_argument(
        "--matrix",
        choices=("none", "minimal", "papi", "all"),
        default="none",
        help="Run a build/runtime matrix instead of a single configuration.",
    )
    parser.add_argument(
        "--require-papi-hardware",
        action="store_true",
        help="Fail instead of skipping when PAPI hardware counters are unavailable.",
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

    if args.test_papi_counters and args.papi == "none":
        parser.error("--test-papi-counters requires --papi system or --papi local")
    if not args.test_wallclock and not args.test_papi_counters:
        parser.error("--test-no-wallclock requires --test-papi-counters")
    if args.require_papi_hardware and not args.test_papi_counters:
        parser.error("--require-papi-hardware requires --test-papi-counters")

    return args


def use_color() -> bool:
    if os.environ.get("NO_COLOR") is not None:
        return False
    if os.environ.get("FORCE_COLOR") is not None:
        return True
    return sys.stdout.isatty()


def print_yellow(message: str) -> None:
    if use_color():
        print(f"{YELLOW}{message}{RESET}")
    else:
        print(message)


def print_yellow_block(lines: list[str]) -> None:
    print()
    for line in lines:
        print_yellow(line)


def print_papi_hardware_access_hint() -> None:
    """Explain how to enable Linux performance-counter access."""
    print_yellow_block(
        [
            "PAPI hardware counters are not available.",
            "On Linux, counter access is controlled by perf_event_paranoid.",
            "Check the current value with:",
            "  cat /proc/sys/kernel/perf_event_paranoid",
            "To permit hardware-counter access, lower it temporarily, for example:",
            "  sudo sysctl -w kernel.perf_event_paranoid=1",
            "Use 0 or -1 only if you understand the additional access this grants.",
            "Containers and virtual machines may also require host-side configuration.",
            "See docs/source/developer_guide/testing.rst for details.",
        ]
    )


def print_papi_enable_hint(args: argparse.Namespace) -> None:
    """Print instructions when the run used wallclock-only tests."""
    if args.test_papi_counters:
        return

    lines = ["PAPI hardware-counter tests were not enabled."]

    if args.papi == "none":
        lines.extend(
            [
                "To enable them with the test-only local PAPI installation:",
                "  python3 tests/run_tests.py --papi local --test-papi-counters",
                "Or use an existing PAPI installation:",
                "  python3 tests/run_tests.py --papi system "
                "--papi-prefix /path/to/papi --test-papi-counters",
            ]
        )
    else:
        command = f"python3 tests/run_tests.py --papi {args.papi}"
        if args.papi_prefix:
            command += f" --papi-prefix {shlex.quote(args.papi_prefix)}"
        command += " --test-papi-counters"
        lines.extend(
            [
                "PAPI support is built in, but hardware-counter tests were not requested.",
                "Add --test-papi-counters to run them:",
                f"  {command}",
            ]
        )

    lines.append(
        "Add --test-no-wallclock to test PAPI counters without wallclock timing."
    )
    lines.append("See docs/source/developer_guide/testing.rst for details.")
    print_yellow_block(lines)


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

    if args.matrix != "none":
        return run_matrix(args, python)

    env, papi_prefix = build_environment(args)

    if not args.keep_build and not args.no_build:
        clean_build_artifacts(args, env)

    papi_hardware_unavailable = False
    required_papi_hardware_failed = False

    try:
        if not args.no_configure:
            configure(args, env, papi_prefix)

        if not args.no_build:
            build(args, env)

        stale_config = TESTS_DIR / "config_tests.mk"
        stale_config.unlink(missing_ok=True)

        test_env = prepare_test_environment(args, env)

        if args.test_papi_counters and args.papi == "local":
            doctor = TESTS_DIR / "tools" / "papi-local" / "doctor.sh"
            doctor_env = test_env.copy()
            doctor_env["PERF_REGIONS_TEST_PAPI_QUIET"] = "1"
            result = subprocess.run(
                [str(doctor), "--strict"],
                cwd=REPO_ROOT,
                env=doctor_env,
                text=True,
                check=False,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
            if result.returncode != 0:
                if result.stdout:
                    print(result.stdout, end="", flush=True)
                    if not result.stdout.endswith("\n"):
                        print(flush=True)
                if args.require_papi_hardware:
                    print(
                        "ERROR: PAPI hardware counters are required but unavailable.",
                        flush=True,
                    )
                    required_papi_hardware_failed = True
                else:
                    print(
                        "PAPI hardware counters are unavailable; "
                        "skipping the hardware-counter test run.",
                        flush=True,
                    )
                    papi_hardware_unavailable = True
            elif args.verbose_build and result.stdout:
                print(result.stdout, end="", flush=True)
                if not result.stdout.endswith("\n"):
                    print(flush=True)

        if not papi_hardware_unavailable:
            run(pytest_command(args, python), cwd=TESTS_DIR, env=test_env)
    finally:
        if not args.keep_build and not args.no_build:
            clean_build_artifacts(args, env)

    if papi_hardware_unavailable or required_papi_hardware_failed:
        print_papi_hardware_access_hint()
    print_papi_enable_hint(args)
    if required_papi_hardware_failed:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
