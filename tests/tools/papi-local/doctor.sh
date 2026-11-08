#!/usr/bin/env bash
#
# Validate the locally installed, test-only PAPI.
#
# This script never uses sudo and never changes kernel settings.

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

STRICT=0
for arg in "$@"; do
    case "$arg" in
        --strict) STRICT=1 ;;
        -h|--help)
            cat <<'USAGE'
Usage: doctor.sh [--strict]

Checks the local PAPI installation, tools, and basic counter access.
With --strict, failure to read PAPI_TOT_CYC is treated as an error.
USAGE
            exit 0
            ;;
        *) echo "Unknown option: $arg" >&2; exit 1 ;;
    esac
done

# shellcheck source=env.sh
source "$SCRIPT_DIR/env.sh"

CC_VALUE="${CC:-cc}"
if ! command -v "$CC_VALUE" >/dev/null 2>&1; then
    echo "ERROR: C compiler not found: $CC_VALUE" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

cat > "$TMP_DIR/papi_check.c" <<'C_EOF'
#include <stdio.h>
#include <papi.h>

int main(void)
{
    int ret = PAPI_library_init(PAPI_VER_CURRENT);
    if (ret != PAPI_VER_CURRENT && ret > 0) {
        fprintf(stderr, "PAPI version mismatch: %d\n", ret);
        return 2;
    }
    if (ret < 0) {
        fprintf(stderr, "PAPI_library_init failed: %s\n", PAPI_strerror(ret));
        return 2;
    }

    int event_set = PAPI_NULL;
    ret = PAPI_create_eventset(&event_set);
    if (ret != PAPI_OK) {
        fprintf(stderr, "PAPI_create_eventset failed: %s\n", PAPI_strerror(ret));
        return 2;
    }

    ret = PAPI_add_named_event(event_set, "PAPI_TOT_CYC");
    if (ret != PAPI_OK) {
        fprintf(stderr, "PAPI_add_named_event(PAPI_TOT_CYC) failed: %s\n", PAPI_strerror(ret));
        PAPI_destroy_eventset(&event_set);
        return 3;
    }

    ret = PAPI_start(event_set);
    if (ret != PAPI_OK) {
        fprintf(stderr, "PAPI_start failed: %s\n", PAPI_strerror(ret));
        PAPI_destroy_eventset(&event_set);
        return 4;
    }

    long long value = 0;
    ret = PAPI_stop(event_set, &value);
    if (ret != PAPI_OK) {
        fprintf(stderr, "PAPI_stop failed: %s\n", PAPI_strerror(ret));
        PAPI_destroy_eventset(&event_set);
        return 4;
    }

    PAPI_destroy_eventset(&event_set);

    if (value <= 0) {
        fprintf(stderr, "PAPI_TOT_CYC returned no positive count (%lld)\n", value);
        return 5;
    }

    printf("PAPI_TOT_CYC=%lld\n", value);
    return 0;
}
C_EOF

echo "Local PAPI prefix: $PAPI_PREFIX"
if [[ -f "$PAPI_PREFIX/.perf_regions_papi_install" ]]; then
    sed 's/^/  /' "$PAPI_PREFIX/.perf_regions_papi_install"
fi

status=0
WARNINGS=0

for tool in papi_avail papi_native_avail; do
    if [[ -x "$PAPI_PREFIX/bin/$tool" ]]; then
        echo "Found $PAPI_PREFIX/bin/$tool"
    else
        echo "WARNING: $PAPI_PREFIX/bin/$tool not found" >&2
        status=1
    fi
done

# Compile and link against the local installation.
# shellcheck disable=SC2086
if ! "$CC_VALUE" $PAPI_CFLAGS "$TMP_DIR/papi_check.c" $PAPI_LDFLAGS -o "$TMP_DIR/papi_check"; then
    echo "ERROR: failed to compile/link the PAPI check program" >&2
    exit 1
fi

echo "Linked PAPI library:"
if command -v ldd >/dev/null 2>&1; then
    ldd "$TMP_DIR/papi_check" | grep -i 'libpapi' | sed 's/^/  /' || echo "  (libpapi is statically linked or not listed by ldd)"
fi

# Runtime counter access is often restricted in containers/VMs.
set +e
"$TMP_DIR/papi_check"
runtime_status=$?
set -e

if [[ $runtime_status -eq 0 ]]; then
    echo "Basic PAPI counter access: OK"
else
    echo "WARNING: basic PAPI counter access failed (exit code $runtime_status)" >&2
    WARNINGS=1
    if [[ $STRICT -eq 1 ]]; then
        status=1
    fi
fi

if command -v "$PAPI_PREFIX/bin/papi_avail" >/dev/null 2>&1; then
    set +e
    "$PAPI_PREFIX/bin/papi_avail" -a >/dev/null 2>&1
    avail_status=$?
    set -e
    if [[ $avail_status -eq 0 ]]; then
        echo "papi_avail: OK"
    else
        echo "WARNING: papi_avail returned exit code $avail_status" >&2
        WARNINGS=1
        if [[ $STRICT -eq 1 ]]; then
            status=1
        fi
    fi
fi

if [[ -r /proc/sys/kernel/perf_event_paranoid ]]; then
    paranoid="$(cat /proc/sys/kernel/perf_event_paranoid)"
    echo "perf_event_paranoid: $paranoid"
    if [[ "$paranoid" -gt 2 ]]; then
        WARNINGS=1
        cat >&2 <<'MSG'
WARNING: perf_event_paranoid is high; many hardware events may be unavailable.
         This script will not change kernel settings.  If your test environment
         permits it, a developer can adjust it manually, for example:
             sudo sysctl kernel.perf_event_paranoid=1
MSG
    fi
fi

if command -v "$PAPI_PREFIX/bin/papi_native_avail" >/dev/null 2>&1; then
    set +e
    "$PAPI_PREFIX/bin/papi_native_avail" >/dev/null 2>&1
    native_status=$?
    set -e
    if [[ $native_status -eq 0 ]]; then
        echo "papi_native_avail: OK"
    else
        echo "WARNING: papi_native_avail returned exit code $native_status" >&2
        WARNINGS=1
        if [[ $STRICT -eq 1 ]]; then
            status=1
        fi
    fi
fi

if [[ $status -ne 0 ]]; then
    echo "PAPI test environment has errors." >&2
elif [[ $WARNINGS -ne 0 ]]; then
    echo "PAPI test environment is installed, but hardware counter access is limited." >&2
    echo "Use PERF_REGIONS_COUNTERS=WALLCLOCKTIME for wallclock-only tests." >&2
else
    echo "PAPI test environment looks usable."
fi

exit "$status"
