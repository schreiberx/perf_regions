#!/usr/bin/env python3
import argparse
import subprocess
import os
import sys


def main():
    parser = argparse.ArgumentParser(description="Run all tests")
    parser.add_argument("--no-papi", action="store_true", help="Disable PAPI counters")
    args = parser.parse_args()

    # Define list of test commands
    # (name, command_args)
    test_commands = [
        (
            "array_test_c",
            [sys.executable, os.path.join("array_test_c", "run_tests.py")],
        ),
        ("skip_test_c", [sys.executable, os.path.join("skip_test_c", "run_tests.py")]),
        (
            "array_test_f",
            [sys.executable, os.path.join("array_test_f", "run_tests.py")],
        ),
        (
            "stream_benchmark",
            [sys.executable, os.path.join("stream_benchmark", "run_tests.py")],
        ),
        (
            "nemo_test_f_direct",
            [sys.executable, os.path.join("nemo_test_f_direct", "run_tests.py")],
        ),
        ("nemo_test_f", [sys.executable, os.path.join("nemo_test_f", "run_tests.py")]),
    ]

    base_dir = os.path.dirname(os.path.abspath(__file__))
    failed_tests = []

    print(f"Running all tests from {base_dir}...")

    # Set environment for no-papi
    env = os.environ.copy()
    if args.no_papi:
        print("PAPI counters disabled (setting PERF_REGIONS_COUNTERS=WALLCLOCKTIME).")
        env["PERF_REGIONS_COUNTERS"] = "WALLCLOCKTIME"

    for name, cmd in test_commands:
        print(f"\n[{name}] Executing...")

        try:
            # Run in the base_dir (tests/)
            subprocess.check_call(cmd, cwd=base_dir, env=env)
            print(f"[{name}] PASSED")

        except subprocess.CalledProcessError:
            print(f"[{name}] FAILED")
            failed_tests.append(name)
        except Exception as e:
            print(f"[{name}] ERROR: {e}")
            failed_tests.append(name)

    if failed_tests:
        print(f"\nSome tests failed: {failed_tests}")
        sys.exit(1)
    else:
        print("\nAll tests passed successfully.")
        sys.exit(0)


if __name__ == "__main__":
    main()
