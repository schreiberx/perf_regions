#!/usr/bin/env bash
#
# Compatibility helper. Prefer:
#   python3 tests/run_tests.py --no-mpi --papi local --no-fortran --test-papi-counters \
#       --pytest-target bandwidth_monitor_papi
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$SCRIPT_DIR/../run_tests.py" \
    --no-mpi \
    --no-fortran \
    --papi local \
    --test-papi-counters \
    --pytest-target bandwidth_monitor_papi \
    "$@"
