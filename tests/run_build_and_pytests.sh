#!/usr/bin/env bash
#
# Compatibility wrapper. New code should call tests/run_tests.py directly.
#
# Examples:
#   python3 tests/run_tests.py --wallclock-only
#   python3 tests/run_tests.py --mpi --papi local
#   python3 tests/run_tests.py --cc clang --no-mpi

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

if [[ $# -eq 0 ]]; then
    # Preserve the old "install dependencies and run tests" behaviour.
    exec python3 "$SCRIPT_DIR/run_tests.py" --venv
fi

exec python3 "$SCRIPT_DIR/run_tests.py" "$@"
