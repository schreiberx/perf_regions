#!/usr/bin/env bash
#
# Activate a locally installed, test-only PAPI.
#
# Usage:
#   source tests/tools/papi-local/env.sh
#
# This file must be sourced, not executed.

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    echo "ERROR: source this file instead of executing it:" >&2
    echo "    source ${BASH_SOURCE[0]}" >&2
    exit 1
fi

_perf_regions_papi_env_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
_perf_regions_repo_root="$(cd -- "$_perf_regions_papi_env_dir/../../.." && pwd)"
_perf_regions_deps_dir="${PERF_REGIONS_TEST_DEPS_DIR:-$_perf_regions_repo_root/tests/.deps}"

_perf_regions_papi_prefix="${PERF_REGIONS_TEST_PAPI_PREFIX:-${PAPI_PREFIX:-}}"
if [[ -z "$_perf_regions_papi_prefix" ]]; then
    _perf_regions_papi_prefix="$_perf_regions_deps_dir/papi/current"
fi

# Resolve the "current" symlink for nicer diagnostics.
if [[ -L "$_perf_regions_papi_prefix" ]]; then
    _perf_regions_papi_prefix="$(readlink -f "$_perf_regions_papi_prefix" 2>/dev/null || printf '%s' "$_perf_regions_papi_prefix")"
fi

_perf_regions_marker="$_perf_regions_papi_prefix/.perf_regions_papi_install"
if [[ "$_perf_regions_papi_prefix" == *[[:space:]]* ]]; then
    echo "ERROR: PAPI prefix must not contain whitespace:" >&2
    echo "    $_perf_regions_papi_prefix" >&2
    unset _perf_regions_papi_env_dir _perf_regions_repo_root _perf_regions_deps_dir \
          _perf_regions_papi_prefix _perf_regions_marker
    return 1
fi

if [[ ! -f "$_perf_regions_marker" ]]; then
    echo "ERROR: no local test PAPI installation found at:" >&2
    echo "    $_perf_regions_papi_prefix" >&2
    echo "Install it first with:" >&2
    echo "    tests/tools/papi-local/install.sh" >&2
    unset _perf_regions_papi_env_dir _perf_regions_repo_root _perf_regions_deps_dir \
          _perf_regions_papi_prefix _perf_regions_marker
    return 1
fi

if [[ ! -f "$_perf_regions_papi_prefix/include/papi.h" ]]; then
    echo "ERROR: PAPI installation is missing include/papi.h:" >&2
    echo "    $_perf_regions_papi_prefix" >&2
    unset _perf_regions_papi_env_dir _perf_regions_repo_root _perf_regions_deps_dir \
          _perf_regions_papi_prefix _perf_regions_marker
    return 1
fi

_perf_regions_libdir=""
if [[ -f "$_perf_regions_papi_prefix/lib64/libpapi.so" || -f "$_perf_regions_papi_prefix/lib64/libpapi.a" ]]; then
    _perf_regions_libdir="lib64"
elif [[ -f "$_perf_regions_papi_prefix/lib/libpapi.so" || -f "$_perf_regions_papi_prefix/lib/libpapi.a" ]]; then
    _perf_regions_libdir="lib"
else
    echo "ERROR: no libpapi found below:" >&2
    echo "    $_perf_regions_papi_prefix" >&2
    unset _perf_regions_papi_env_dir _perf_regions_repo_root _perf_regions_deps_dir \
          _perf_regions_papi_prefix _perf_regions_marker _perf_regions_libdir
    return 1
fi

_perf_regions_prepend_path() {
    local name="$1"
    shift
    local current="${!name:-}"
    local dir
    for dir in "$@"; do
        [[ -d "$dir" ]] || continue
        case ":$current:" in
            *":$dir:"*) ;;
            *) current="$dir${current:+:$current}" ;;
        esac
    done
    export "$name=$current"
}

export PERF_REGIONS_TEST_PAPI=1
export PERF_REGIONS_TEST_PAPI_PREFIX="$_perf_regions_papi_prefix"
export PAPI_PREFIX="$_perf_regions_papi_prefix"
export PAPI_ROOT="$_perf_regions_papi_prefix"

_perf_regions_prepend_path PATH "$PAPI_PREFIX/bin"
_perf_regions_prepend_path LD_LIBRARY_PATH "$PAPI_PREFIX/lib" "$PAPI_PREFIX/lib64"
_perf_regions_prepend_path PKG_CONFIG_PATH "$PAPI_PREFIX/lib/pkgconfig" "$PAPI_PREFIX/lib64/pkgconfig"

export PAPI_CFLAGS="-I$PAPI_PREFIX/include"
export PAPI_LDFLAGS="-L$PAPI_PREFIX/$_perf_regions_libdir -lpapi"

# Conservative default for tests.  Users can override this before sourcing or
# by exporting PERF_REGIONS_COUNTERS afterwards.
if [[ -z "${PERF_REGIONS_COUNTERS:-}" ]]; then
    export PERF_REGIONS_COUNTERS="PAPI_TOT_CYC,WALLCLOCKTIME"
fi

if [[ "${PERF_REGIONS_TEST_PAPI_QUIET:-0}" != "1" ]]; then
    printf 'Activated test PAPI %s\n' "$PAPI_PREFIX"
    printf '  PAPI_CFLAGS=%s\n' "$PAPI_CFLAGS"
    printf '  PAPI_LDFLAGS=%s\n' "$PAPI_LDFLAGS"
    printf '  PERF_REGIONS_COUNTERS=%s\n' "$PERF_REGIONS_COUNTERS"
    printf 'Run %s/doctor.sh to validate counter access.\n' "$_perf_regions_papi_env_dir"
fi

unset -f _perf_regions_prepend_path
unset _perf_regions_papi_env_dir _perf_regions_repo_root _perf_regions_deps_dir \
      _perf_regions_papi_prefix _perf_regions_marker _perf_regions_libdir
