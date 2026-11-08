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
CASE_RESULTS=()
CASE_DETAILS=()
CASE_PASS=0
CASE_FAIL=0
CASE_SKIP=0

COLOR=0
C_RESET=""
C_BOLD=""
C_CYAN=""
C_GREEN=""
C_RED=""
C_YELLOW=""
if [[ -n "${FORCE_COLOR:-}" || ( -t 1 && -z "${NO_COLOR:-}" ) ]]; then
    COLOR=1
    C_RESET=$'\033[0m'
    C_BOLD=$'\033[1m'
    C_CYAN=$'\033[1;36m'
    C_GREEN=$'\033[1;32m'
    C_RED=$'\033[1;31m'
    C_YELLOW=$'\033[1;33m'
    export PY_COLORS=1
fi

run_case() {
    local description="$1"
    local compile_options="$2"
    local runtime_options="$3"
    shift 3
    local status summary label log clean_log details

    printf '\n'
    printf '%s================================================================%s\n' "$C_CYAN" "$C_RESET"
    printf '%s  %s%s\n' "$C_BOLD" "$description" "$C_RESET"
    printf '  Compile: %s\n' "$compile_options"
    printf '  Runtime: %s\n' "$runtime_options"
    printf '%s================================================================%s\n' "$C_CYAN" "$C_RESET"

    log="$(mktemp)"
    set +e
    "$PYTHON" tests/run_tests.py "$@" --pytest-args=-q "${EXTRA_ARGS[@]}" 2>&1 | tee "$log"
    status=${PIPESTATUS[0]}
    set -e

    clean_log="$(mktemp)"
    sed -E $'s/\x1B\\[[0-9;]*[A-Za-z]//g' "$log" > "$clean_log"

    summary="$(grep -E '^[=]+ .*(passed|failed|error|no tests ran)' "$clean_log" | tail -n 1 | sed -E 's/^=+[[:space:]]*//; s/[[:space:]]*=+$//' || true)"
    if [[ -z "$summary" ]]; then
        if grep -q "skipping the hardware-counter test run" "$clean_log"; then
            summary="SKIPPED (PAPI hardware unavailable)"
        else
            summary="no pytest summary"
        fi
    fi

    details="$(grep -E '^(SKIPPED|FAILED|ERROR)([[:space:]]|\[)' "$clean_log" || true)"

    if [[ "$status" -eq 0 ]]; then
        if [[ "$summary" == SKIPPED* ]]; then
            label="SKIP"
            CASE_SKIP=$((CASE_SKIP + 1))
        else
            label="PASS"
            CASE_PASS=$((CASE_PASS + 1))
        fi
    else
        label="FAIL"
        CASE_FAIL=$((CASE_FAIL + 1))
    fi

    CASE_RESULTS+=("$label|$description|$summary")
    CASE_DETAILS+=("$details")
    rm -f "$log" "$clean_log"
}

if command -v mpicc >/dev/null 2>&1 && command -v mpif90 >/dev/null 2>&1; then
    mpi_modes=(--mpi --no-mpi)
else
    mpi_modes=(--no-mpi)
    printf 'MPI compilers not found; skipping MPI combinations.\n'
fi

for mpi in "${mpi_modes[@]}"; do
    if [[ "$mpi" == "--no-mpi" ]]; then
        mpi_label="no-mpi"
    else
        mpi_label="mpi"
    fi

    if [[ "$mpi_label" == "mpi" ]]; then
        mpi_compile="MPI enabled"
    else
        mpi_compile="MPI disabled"
    fi

    run_case "$mpi_label / PAPI counters + wallclock" \
        "$mpi_compile, PAPI enabled ($PAPI_MODE)" \
        "PAPI counters + wallclock" \
        "$mpi" --papi "$PAPI_MODE" --test-papi-counters --test-wallclock

    run_case "$mpi_label / PAPI counters only" \
        "$mpi_compile, PAPI enabled ($PAPI_MODE)" \
        "PAPI counters only" \
        "$mpi" --papi "$PAPI_MODE" --test-papi-counters --test-no-wallclock

    run_case "$mpi_label / PAPI build / wallclock" \
        "$mpi_compile, PAPI enabled ($PAPI_MODE)" \
        "Wallclock only" \
        "$mpi" --papi "$PAPI_MODE" --test-wallclock

    run_case "$mpi_label / no PAPI / wallclock" \
        "$mpi_compile, PAPI disabled" \
        "Wallclock only" \
        "$mpi" --papi none --test-wallclock
done

printf '\n'
printf '%s================================================================%s\n' "$C_CYAN" "$C_RESET"
printf '%s  Test matrix summary%s\n' "$C_BOLD" "$C_RESET"
printf '%s================================================================%s\n' "$C_CYAN" "$C_RESET"
printf '%-6s  %-48s  %s\n' "STATUS" "CASE" "PYTEST RESULT"
printf '%s\n' '----------------------------------------------------------------'
for i in "${!CASE_RESULTS[@]}"; do
    IFS='|' read -r status description summary <<< "${CASE_RESULTS[$i]}"
    case "$status" in
        PASS) status_color="$C_GREEN" ;;
        FAIL) status_color="$C_RED" ;;
        SKIP) status_color="$C_YELLOW" ;;
        *) status_color="" ;;
    esac
    printf '%s' "$status_color"
    printf '%-6s' "$status"
    printf '%s' "$C_RESET"
    printf '  %-48s  %s\n' "$description" "$summary"
    if [[ -n "${CASE_DETAILS[$i]}" ]]; then
        while IFS= read -r detail; do
            printf '        %s\n' "$detail"
        done <<< "${CASE_DETAILS[$i]}"
    fi
done
printf '%s\n' '----------------------------------------------------------------'
printf 'Cases: %d passed, %d failed, %d skipped, %d total\n' \
    "$CASE_PASS" "$CASE_FAIL" "$CASE_SKIP" "$((CASE_PASS + CASE_FAIL + CASE_SKIP))"

if [[ "$CASE_FAIL" -ne 0 ]]; then
    printf '\n%sTest matrix completed with failures.%s\n' "$C_RED" "$C_RESET"
    exit 1
fi

printf '\n%sAll test combinations completed successfully.%s\n' "$C_GREEN" "$C_RESET"
