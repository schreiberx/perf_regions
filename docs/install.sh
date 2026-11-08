#!/usr/bin/env bash
#
# Install the documentation dependencies and build the PerfRegions docs.
#
# The script uses a repository-local virtual environment by default, so it does
# not modify the system Python installation.

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"

PYTHON="${PYTHON:-python3}"
VENV_DIR="${DOCS_VENV_DIR:-$REPO_ROOT/.venv}"
BUILD=1
CLEAN=0
SPHINXOPTS="${SPHINXOPTS:--j auto}"

usage() {
    cat <<'USAGE'
Usage: docs/install.sh [options]

Create a local documentation virtual environment, install the requirements,
and build the HTML documentation.

Options:
  --venv DIR       Virtual environment directory (default: <repo>/.venv)
  --python CMD     Python executable used to create the environment
  --no-build       Install dependencies only; do not build the docs
  --clean          Remove docs/build before building
  -h, --help       Show this help

Environment:
  PYTHON           Python executable (default: python3)
  DOCS_VENV_DIR    Override the virtual environment directory
  SPHINXOPTS       Extra Sphinx options (default: -j auto)
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --venv)
            VENV_DIR="$2"
            shift 2
            ;;
        --python)
            PYTHON="$2"
            shift 2
            ;;
        --no-build)
            BUILD=0
            shift
            ;;
        --clean)
            CLEAN=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

if ! command -v "$PYTHON" >/dev/null 2>&1; then
    echo "ERROR: Python executable not found: $PYTHON" >&2
    exit 1
fi

if [[ ! -x "$VENV_DIR/bin/python" ]]; then
    echo "Creating documentation virtual environment: $VENV_DIR"
    "$PYTHON" -m venv "$VENV_DIR"
fi

VENV_PYTHON="$VENV_DIR/bin/python"
VENV_SPHINXBUILD="$VENV_DIR/bin/sphinx-build"

echo "Installing documentation requirements"
"$VENV_PYTHON" -m pip install --disable-pip-version-check \
    -r "$SCRIPT_DIR/requirements.txt"

if [[ "$BUILD" -eq 0 ]]; then
    echo "Documentation dependencies installed."
    echo "Activate with: source $VENV_DIR/bin/activate"
    exit 0
fi

if [[ "$CLEAN" -eq 1 ]]; then
    make -C "$SCRIPT_DIR" clean
fi

echo "Building documentation"
make -C "$SCRIPT_DIR" html \
    SPHINXBUILD="$VENV_SPHINXBUILD" \
    SPHINXOPTS="$SPHINXOPTS"

echo "Documentation written to: $SCRIPT_DIR/build/html"
