

# 2026-03-03

- Hardened region-ID validation and nested-region bookkeeping
  (`PERF_NESTED_REGIONS_MAX` is now 128); invalid IDs no longer cause
  out-of-bounds accesses.
- Made `PERF_TIMING_POSIX`, `PERF_COUNTERS_NESTED`, and related switches use
  consistent `#if` handling. The `gettimeofday()` fallback now compiles and
  works when POSIX timers are unavailable.
- `perf_regions_init()`/`perf_regions_finalize()` are idempotent, and PAPI is
  shut down and fully cleaned up.
- MPI reductions now work with PAPI disabled and with PAPI-only runs. They
  also handle regions that only exist on a subset of ranks without deadlock.
- JSON output is escaped correctly and reports zero for min/max when a region
  has no valid samples.
- The public headers no longer expose configuration-dependent struct layouts;
  build options are recorded in a generated `perf_regions_config.h`.
- Fixed `uninstall` so it no longer removes the whole include directory, and
  added `DESTDIR` support.
- Restored NEMO `timing_start`/`timing_stop` instrumentation support.
- Fixed several test/build issues (test assertions, duplicate cleanup,
  `make tests`, dependency tracking, bandwidth-monitor event parsing).
- Added test-only local PAPI tooling under `tests/tools/papi-local/` so PAPI
  can be installed under `tests/.deps/` without touching system directories.

# 2025-02-05

- Started changelog

- Using `pytest` for test cases.

- All test cases have been cleaned up:
    - `src`: source code
    - `bin`: scripts
    - `build`: for building process
    - `test_data`: Verification for changes with `perf_region`
    - `pytests.py`: All test cases for this test

- `perf_region` library names now depend on options:
    - MPI: `_mpi`
    - PAPI: `_papi`
    - Fortran: `_fort`
    E.g., enabling a build with MPI and without PAPI would result in `perf_region_mpi.so`.

- Thanks to Thomas Gruber for discussions on performance counters.

