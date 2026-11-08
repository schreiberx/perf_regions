#!/usr/bin/env bash
#
# Run the full PAPI/wallclock test matrix sequentially.
#
# The script runs each case in a separate run_tests.py invocation because the
# test runner reconfigures, rebuilds, and cleans the shared build directories.
#
# Extra arguments are forwarded to run_tests.py. Do not pass --mpi/--no-mpi,
# --papi, --test-papi-counters, or the wallclock options: this script controls
# those options. Pytest is run in quiet mode; pass your own --pytest-args to
# override this. Set PAPI_MODE=system to use a system PAPI installation instead
# of the test-local one; pass --papi-prefix through the extra arguments if
# needed.

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"

cd "$REPO_ROOT"

PYTHON="${PYTHON:-python3}"
PAPI_MODE="${PAPI_MODE:-local}"
EXTRA_ARGS=("$@")

run_case() {
    local description="$1"
    shift

    printf '\n'
    printf '================================================================\n'
    printf '  %s\n' "$description"
    printf '================================================================\n'

    "$PYTHON" tests/run_tests.py "$@" --pytest-args=-q "${EXTRA_ARGS[@]}"
}

mpi_modes=(--no-mpi)
if command -v mpicc >/dev/null 2>&1 && command -v mpif90 >/dev/null 2>&1; then
    mpi_modes+=(--mpi)
else
    printf 'MPI compilers not found; skipping MPI combinations.\n'
fi

for mpi in "${mpi_modes[@]}"; do
    run_case "$mpi / no PAPI / wallclock" \
        "$mpi" --papi none --test-wallclock

    run_case "$mpi / PAPI build / wallclock" \
        "$mpi" --papi "$PAPI_MODE" --test-wallclock

    run_case "$mpi / PAPI counters + wallclock" \
        "$mpi" --papi "$PAPI_MODE" --test-papi-counters --test-wallclock

    run_case "$mpi / PAPI counters only" \
        "$mpi" --papi "$PAPI_MODE" --test-papi-counters --test-no-wallclock
done

printf '\nAll test combinations completed.\n'
