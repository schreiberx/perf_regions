Changelog
=========

2026-03-03
----------

- Hardened region-ID validation and nested-region bookkeeping
  (``PERF_NESTED_REGIONS_MAX`` is now 128); invalid IDs no longer cause
  out-of-bounds accesses.
- Made ``PERF_TIMING_POSIX``, ``PERF_COUNTERS_NESTED``, and related switches use
  consistent ``#if`` handling. The ``gettimeofday()`` fallback now compiles and
  works when POSIX timers are unavailable.
- ``perf_regions_init()`` and ``perf_regions_finalize()`` are idempotent, and
  PAPI is shut down and fully cleaned up.
- MPI reductions now work with PAPI disabled and with PAPI-only runs. They also
  handle regions that only exist on a subset of ranks without deadlock.
- JSON output is escaped correctly and reports zero for min/max when a region
  has no valid samples.
- The public headers no longer expose configuration-dependent struct layouts;
  build options are recorded in a generated ``perf_regions_config.h``.
- Fixed ``uninstall`` so it no longer removes the whole include directory, and
  added ``DESTDIR`` support.
- Restored NEMO ``timing_start``/``timing_stop`` instrumentation support.
- Fixed several test and build issues (test assertions, duplicate cleanup,
  ``make tests``, dependency tracking, bandwidth-monitor event parsing).
- Added test-only local PAPI tooling under ``tests/tools/papi-local/`` so PAPI
  can be installed under ``tests/.deps/`` without touching system directories.
- Added a GitHub Actions CI workflow covering GCC, Clang, MPI, PAPI, Fortran,
  and an optional NVIDIA HPC SDK container job.
- Simplified test execution around ``tests/run_tests.py``, shared pytest
  fixtures, common test Makefile definitions, and a single instrumentation
  entry point. CI and local runs now use the same runner.
- Tightened the Fortran interface: removed the unnecessary ``//achar(0)``
  concatenation at generated call sites, used explicit ``C_CHAR``/``C_INT``
  kinds, and removed an unused defines-header include from the Fortran module.

2025-02-05
----------

- Started the changelog.
- Switched the test cases to ``pytest``.
- Cleaned up the test layout around ``src``, ``bin``, ``build``,
  ``test_data``, and the pytest test modules.
- Made the ``perf_region`` library names depend on the enabled options:

  - MPI: ``_mpi``
  - PAPI: ``_papi``
  - Fortran: ``_fort``

  For example, enabling MPI without PAPI results in
  ``libperf_regions_mpi.so``.
- Thanks to Thomas Gruber for discussions on performance counters.
