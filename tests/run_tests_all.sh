#!/usr/bin/env bash
#
# Run the full compile/runtime test matrix through the Python test driver.
#
# The Python driver builds each compile variant once and reuses it for the
# different runtime counter/timing modes.

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"

cd "$REPO_ROOT"

PYTHON="${PYTHON:-python3}"
PAPI_MODE="${PAPI_MODE:-local}"

exec "$PYTHON" tests/run_tests.py \
    --matrix all \
    --papi "$PAPI_MODE" \
    "$@"
