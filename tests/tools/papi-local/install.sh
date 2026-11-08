#!/usr/bin/env bash
#
# Install a test-only PAPI into a repo-local prefix.
#
# This script never uses sudo and refuses to install outside tests/.deps by
# default.  It is intended for development and CI only.

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/../../.." && pwd)"

DEFAULT_PAPI_VERSION="7.2.0"
DEFAULT_PAPI_TAG="papi-7-2-0-t"
DEFAULT_PAPI_SHA256="a9bff89ccf39915d729e08ae0a0c6a71ce0ebbe98411e9a2eb3c83c8db0af39c"

PAPI_SHA256_EXPLICIT=0
PAPI_URL_EXPLICIT=0
[[ -n "${PAPI_SHA256+x}" ]] && PAPI_SHA256_EXPLICIT=1
[[ -n "${PAPI_URL+x}" ]] && PAPI_URL_EXPLICIT=1

PAPI_VERSION="${PAPI_VERSION:-$DEFAULT_PAPI_VERSION}"
PAPI_TAG="${PAPI_TAG:-}"
PAPI_URL="${PAPI_URL:-}"
PAPI_SHA256="${PAPI_SHA256:-}"

DEPS_DIR="${PERF_REGIONS_TEST_DEPS_DIR:-$REPO_ROOT/tests/.deps}"
CACHE_DIR="$DEPS_DIR/cache"
BUILD_ROOT="$DEPS_DIR/build"
PAPI_ROOT="$DEPS_DIR/papi"
CURRENT_LINK="$PAPI_ROOT/current"

ARCH="$(uname -m)"
CC_VALUE="${CC:-cc}"
COMPILER_TAG="$(basename "$CC_VALUE" | sed -e 's/[^A-Za-z0-9_.-]/_/g' -e 's/_*$//')"
if [[ -z "$COMPILER_TAG" ]]; then
    COMPILER_TAG="compiler"
fi
PREFIX="${PAPI_PREFIX:-$PAPI_ROOT/${PAPI_VERSION}-${ARCH}-${COMPILER_TAG}}"
PREFIX_EXPLICIT=0
[[ -n "${PAPI_PREFIX+x}" ]] && PREFIX_EXPLICIT=1
BUILD_DIR="$BUILD_ROOT/papi-${PAPI_VERSION}-${ARCH}-${COMPILER_TAG}"

JOBS="${JOBS:-}"
if [[ -z "$JOBS" ]]; then
    JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
fi

# PAPI 7.2.0's bundled libpfm4 is not warning-clean with newer GCC versions
# that turn const-qualifier warnings into errors.  This is only a build flag
# for the test-only PAPI copy.
PAPI_BUILD_CFLAGS="${PAPI_BUILD_CFLAGS:-${CFLAGS:-}}"
case " $PAPI_BUILD_CFLAGS " in
    *" -Wno-error=discarded-qualifiers "*) ;;
    *)
        if printf 'int main(void) { return 0; }\n' | \
           "$CC_VALUE" -x c -Wno-error=discarded-qualifiers -c -o /dev/null - 2>/dev/null; then
            PAPI_BUILD_CFLAGS="${PAPI_BUILD_CFLAGS:+$PAPI_BUILD_CFLAGS }-Wno-error=discarded-qualifiers"
        fi
        ;;
esac

FORCE=0
CLEAN=0
CLEAN_CACHE=0
ALLOW_ROOT=0
ALLOW_EXTERNAL_PREFIX=0
TARBALL=""

usage() {
    cat <<'USAGE'
Usage: install.sh [options]

Build and install PAPI into a local, test-only prefix.

Options:
  --version VERSION        PAPI version (default: 7.2.0)
  --tag TAG                GitHub release tag (default for 7.2.0: papi-7-2-0-t)
  --url URL                tarball URL
  --sha256 HEX             expected tarball SHA256 (empty disables verification)
  --tarball FILE           use an existing tarball instead of downloading
  --prefix DIR             install prefix (default under tests/.deps/papi/)
  --jobs N                 parallel build jobs
  --force                  reinstall even if the marker matches
  --clean                  remove this local installation and its build tree
  --clean-cache            with --clean, also remove the download cache
  --allow-root             allow running as root (containers only)
  --allow-external-prefix  allow a prefix outside tests/.deps
  -h, --help               show this help

Environment:
  PAPI_VERSION, PAPI_TAG, PAPI_URL, PAPI_SHA256, PAPI_PREFIX
  PERF_REGIONS_TEST_DEPS_DIR   override the default tests/.deps directory
  PAPI_CONFIGURE_ARGS          extra arguments passed to PAPI's configure
  PAPI_BUILD_CFLAGS            CFLAGS for building PAPI (default: CFLAGS)

After installation:
  source tests/tools/papi-local/env.sh
  tests/tools/papi-local/doctor.sh
USAGE
}

die() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

info() {
    printf 'INFO: %s\n' "$*"
}

warn() {
    printf 'WARNING: %s\n' "$*" >&2
}

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "Required command not found: $1"
}

realpath_m() {
    if command -v realpath >/dev/null 2>&1; then
        realpath -m "$1"
    else
        # Fallback for systems without realpath -m.
        (cd -- "$(dirname -- "$1")" 2>/dev/null && printf '%s/%s\n' "$(pwd -P)" "$(basename -- "$1")") \
            || printf '%s\n' "$1"
    fi
}

is_under_deps() {
    local path resolved deps_resolved
    path="$(realpath_m "$1")"
    deps_resolved="$(realpath_m "$DEPS_DIR")"
    case "$path" in
        "$deps_resolved"/*) return 0 ;;
        *) return 1 ;;
    esac
}

safe_remove_under_deps() {
    local path="$1"
    is_under_deps "$path" || die "Refusing to remove path outside tests/.deps: $path"
    rm -rf -- "$path"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version) PAPI_VERSION="$2"; shift 2 ;;
        --tag) PAPI_TAG="$2"; shift 2 ;;
        --url) PAPI_URL="$2"; PAPI_URL_EXPLICIT=1; shift 2 ;;
        --sha256) PAPI_SHA256="$2"; PAPI_SHA256_EXPLICIT=1; shift 2 ;;
        --tarball) TARBALL="$2"; shift 2 ;;
        --prefix) PREFIX="$2"; PREFIX_EXPLICIT=1; shift 2 ;;
        --jobs) JOBS="$2"; shift 2 ;;
        --force) FORCE=1; shift ;;
        --clean) CLEAN=1; shift ;;
        --clean-cache) CLEAN_CACHE=1; shift ;;
        --allow-root) ALLOW_ROOT=1; shift ;;
        --allow-external-prefix) ALLOW_EXTERNAL_PREFIX=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) die "Unknown option: $1 (use --help)" ;;
    esac
done

# Derive tag/URL/checksum/prefix after parsing so --version/--url work correctly.
if [[ "$PREFIX_EXPLICIT" -eq 0 ]]; then
    PREFIX="$PAPI_ROOT/${PAPI_VERSION}-${ARCH}-${COMPILER_TAG}"
fi
BUILD_DIR="$BUILD_ROOT/papi-${PAPI_VERSION}-${ARCH}-${COMPILER_TAG}"

if [[ -z "$PAPI_TAG" ]]; then
    if [[ "$PAPI_VERSION" == "$DEFAULT_PAPI_VERSION" ]]; then
        PAPI_TAG="$DEFAULT_PAPI_TAG"
    else
        PAPI_TAG="papi-${PAPI_VERSION//./-}"
    fi
fi

if [[ -z "$PAPI_URL" ]]; then
    PAPI_URL="https://github.com/icl-utk-edu/papi/releases/download/${PAPI_TAG}/papi-${PAPI_VERSION}.tar.gz"
else
    PAPI_URL_EXPLICIT=1
fi

# Only use the pinned checksum for the default version and URL unless the
# user explicitly supplied a checksum.
if [[ "$PAPI_SHA256_EXPLICIT" -eq 0 ]]; then
    if [[ "$PAPI_VERSION" == "$DEFAULT_PAPI_VERSION" && "$PAPI_URL_EXPLICIT" -eq 0 ]]; then
        PAPI_SHA256="$DEFAULT_PAPI_SHA256"
    else
        PAPI_SHA256=""
    fi
fi

if [[ "${EUID:-$(id -u)}" -eq 0 && "$ALLOW_ROOT" -ne 1 ]]; then
    die "Refusing to run as root. Use --allow-root only in a disposable container."
fi

if [[ "$ALLOW_EXTERNAL_PREFIX" -ne 1 ]] && ! is_under_deps "$PREFIX"; then
    die "Prefix must be under $DEPS_DIR (use --allow-external-prefix to override): $PREFIX"
fi
if [[ "$PREFIX" == *[[:space:]]* ]]; then
    die "PAPI prefix must not contain whitespace: $PREFIX"
fi

if [[ "$CLEAN" -eq 1 ]]; then
    info "Removing local PAPI installation: $PREFIX"
    safe_remove_under_deps "$PREFIX"
    safe_remove_under_deps "$BUILD_DIR"
    if [[ "$CLEAN_CACHE" -eq 1 ]]; then
        safe_remove_under_deps "$CACHE_DIR"
    fi
    if [[ -L "$CURRENT_LINK" ]]; then
        local_target="$(readlink "$CURRENT_LINK" || true)"
        if [[ "$local_target" == "$(basename "$PREFIX")" ]]; then
            rm -f -- "$CURRENT_LINK"
        fi
    fi
    exit 0
fi

need_cmd tar
need_cmd make
need_cmd "${CC:-cc}"
if ! command -v curl >/dev/null 2>&1 && ! command -v wget >/dev/null 2>&1; then
    die "Either curl or wget is required to download PAPI"
fi
if ! command -v sha256sum >/dev/null 2>&1 && ! command -v shasum >/dev/null 2>&1; then
    warn "Neither sha256sum nor shasum found; checksum verification will be skipped"
    PAPI_SHA256=""
fi

MARKER="$PREFIX/.perf_regions_papi_install"
if [[ "$FORCE" -ne 1 && -f "$MARKER" ]] && grep -qx "version=$PAPI_VERSION" "$MARKER"; then
    info "PAPI $PAPI_VERSION is already installed at $PREFIX (use --force to reinstall)"
    if [[ "$PREFIX" == "$PAPI_ROOT"/* ]]; then
        ln -sfn "$(basename "$PREFIX")" "$CURRENT_LINK"
    fi
    exit 0
fi

mkdir -p "$CACHE_DIR" "$BUILD_ROOT" "$PAPI_ROOT"

download_tarball() {
    local dest="$1"
    local tmp="${dest}.part.$$"
    rm -f -- "$tmp"
    info "Downloading PAPI $PAPI_VERSION from $PAPI_URL"
    if command -v curl >/dev/null 2>&1; then
        curl -fL --retry 3 --connect-timeout 20 -o "$tmp" "$PAPI_URL"
    else
        wget -O "$tmp" "$PAPI_URL"
    fi
    mv -f -- "$tmp" "$dest"
}

sha256_of() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    else
        shasum -a 256 "$1" | awk '{print $1}'
    fi
}

if [[ -n "$TARBALL" ]]; then
    [[ -f "$TARBALL" ]] || die "Tarball not found: $TARBALL"
    TARBALL_PATH="$TARBALL"
else
    TARBALL_PATH="$CACHE_DIR/papi-${PAPI_VERSION}.tar.gz"
    if [[ -f "$TARBALL_PATH" && -n "$PAPI_SHA256" ]]; then
        cached_sum="$(sha256_of "$TARBALL_PATH")"
        if [[ "$cached_sum" != "$PAPI_SHA256" ]]; then
            warn "Cached tarball checksum mismatch; downloading a fresh copy"
            rm -f -- "$TARBALL_PATH"
        fi
    fi
    if [[ ! -f "$TARBALL_PATH" ]]; then
        download_tarball "$TARBALL_PATH"
    fi
fi

if [[ -n "$PAPI_SHA256" ]]; then
    actual_sum="$(sha256_of "$TARBALL_PATH")"
    if [[ "$actual_sum" != "$PAPI_SHA256" ]]; then
        die "SHA256 mismatch for $TARBALL_PATH: expected $PAPI_SHA256, got $actual_sum"
    fi
    info "SHA256 verified: $actual_sum"
else
    warn "No SHA256 configured for $TARBALL_PATH; skipping checksum verification"
fi

safe_remove_under_deps "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
info "Extracting PAPI into $BUILD_DIR"
tar -xzf "$TARBALL_PATH" -C "$BUILD_DIR" --strip-components=1

PAPI_SRC_DIR="$BUILD_DIR/src"
if [[ ! -x "$PAPI_SRC_DIR/configure" ]]; then
    if [[ -x "$BUILD_DIR/configure" ]]; then
        PAPI_SRC_DIR="$BUILD_DIR"
    else
        die "PAPI configure script not found in $BUILD_DIR"
    fi
fi

info "Configuring PAPI with prefix $PREFIX"
(
    cd "$PAPI_SRC_DIR"
    export CFLAGS="$PAPI_BUILD_CFLAGS"
    export CXXFLAGS="${CXXFLAGS:-$PAPI_BUILD_CFLAGS}"
    # shellcheck disable=SC2086
    CC="${CC:-cc}" ./configure --prefix="$PREFIX" ${PAPI_CONFIGURE_ARGS:-}
    make -j"$JOBS"
    make install
)

if [[ ! -f "$PREFIX/include/papi.h" ]]; then
    die "PAPI installation did not produce $PREFIX/include/papi.h"
fi

cat > "$MARKER" <<MARKER_EOF
version=$PAPI_VERSION
tag=$PAPI_TAG
url=$PAPI_URL
sha256=$PAPI_SHA256
arch=$ARCH
compiler=$CC_VALUE
build_cflags=$PAPI_BUILD_CFLAGS
prefix=$PREFIX
date=$(date -u +%Y-%m-%dT%H:%M:%SZ)
MARKER_EOF

if [[ "$PREFIX" == "$PAPI_ROOT"/* ]]; then
    ln -sfn "$(basename "$PREFIX")" "$CURRENT_LINK"
fi

info "Installed PAPI $PAPI_VERSION into $PREFIX"
info "Activate it with:"
printf '\n    source %s/env.sh\n\n' "$SCRIPT_DIR"
info "Then validate with:"
printf '\n    %s/doctor.sh\n\n' "$SCRIPT_DIR"
