# Tests for perf regions

Run tests run for all combinations of
- PAPI in {0, 1}
- MPI in {0, 1}


# Pre-configuration

Before running, call `./configure` in the `perf_region` main directory.
MPI and PAPI configurations are skipped automatically if they are not enabled
or not available.


# Test-only local PAPI

If PAPI is not installed on the system, install a dedicated test copy under
`tests/.deps`:

```bash
tests/tools/papi-local/install.sh
source tests/tools/papi-local/env.sh
tests/tools/papi-local/doctor.sh
```

Then configure/build/test against it:

```bash
./configure --with-papi="$PAPI_PREFIX"
./build.sh
cd tests && ./run_build_and_pytests.sh
```

See `tests/tools/papi-local/README.md` for details and cleanup.
