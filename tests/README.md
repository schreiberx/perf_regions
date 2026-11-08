# Tests for perf regions

The canonical entry point is:

```bash
python3 tests/run_tests.py
```

It configures, builds, runs pytest, and cleans up afterwards. By default it
uses wallclock timing, no MPI, and builds both the C and Fortran libraries if a
Fortran compiler is available.

## Quick start

From the repository root:

```bash
# Use the current Python environment
python3 tests/run_tests.py

# Create/use tests/.venv and install test dependencies
python3 tests/run_tests.py --venv
```

The runner installs nothing globally and cleans build artifacts unless
`--keep-build` is used.

## Common configurations

```bash
# Minimal build: no MPI, no PAPI
python3 tests/run_tests.py --no-mpi --papi none

# MPI build
python3 tests/run_tests.py --mpi --papi none

# Use the test-only local PAPI installation
python3 tests/run_tests.py --mpi --papi local

# Use an already installed PAPI prefix
python3 tests/run_tests.py --papi system --papi-prefix /path/to/papi

# Clang C compiler with gfortran for Fortran
python3 tests/run_tests.py --cc clang --cxx clang++ --fc gfortran

# Skip Fortran entirely
python3 tests/run_tests.py --no-fortran

# Short run: exclude slow tests
python3 tests/run_tests.py --fast
```

## PAPI counters

By default, tests use:

```text
PERF_REGIONS_COUNTERS=WALLCLOCKTIME
```

To run hardware-counter tests explicitly:

```bash
python3 tests/run_tests.py --mpi --papi local --papi-hardware
```

This requires working PAPI hardware counter access. On many containers, VMs, or
CI runners these counters are unavailable. The optional GitHub CI hardware step
checks this with:

```bash
source tests/tools/papi-local/env.sh
tests/tools/papi-local/doctor.sh --strict
```

and skips the hardware run if it fails.

## Test-only local PAPI

If PAPI is not installed on the system, install a dedicated test copy under
`tests/.deps`:

```bash
tests/tools/papi-local/install.sh
source tests/tools/papi-local/env.sh
tests/tools/papi-local/doctor.sh
```

Then:

```bash
python3 tests/run_tests.py --mpi --papi local
```

The runner also installs the local PAPI automatically when `--papi local` is
used and no installation exists yet. See
`tests/tools/papi-local/README.md` for details and cleanup.

## Running pytest directly

If the library has already been configured and built, pytest can be run
directly:

```bash
python3 -m pytest tests
```

The shared fixtures live in `tests/conftest.py`. They add the build directory to
`LD_LIBRARY_PATH`, write JSON outputs to pytest temporary directories, and
centralize PAPI/wallclock selection.

Useful options:

```bash
python3 -m pytest tests -m "not slow"
python3 -m pytest tests -m papi_hardware
python3 -m pytest tests/array_test_c -q
```

## Legacy wrapper

`tests/run_build_and_pytests.sh` is kept as a compatibility wrapper. New usage
should call `tests/run_tests.py` directly.
