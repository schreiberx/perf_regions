Testing
=======

The canonical test entry point is:

.. code-block:: console

   python3 tests/run_tests.py

The runner configures the library, builds it, runs pytest, and cleans up
afterwards. By default it uses wallclock timing, no MPI, and builds both the C
and Fortran libraries when a Fortran compiler is available. Configure, build,
and dependency-install output is hidden unless a command fails. Successful
PAPI doctor diagnostics are also suppressed; they are printed when validation
fails or when ``--verbose-build`` is used. Pytest still runs with ``-v`` so each
individual test case is listed as it runs. Pass ``--pytest-args '-q'`` for
quieter pytest output.

Quick start
-----------

From the repository root:

.. code-block:: console

   # Use the current Python environment.
   python3 tests/run_tests.py

   # Create or reuse tests/.venv and install test dependencies.
   python3 tests/run_tests.py --venv

The runner does not install anything globally and removes build artifacts unless
``--keep-build`` is used.

Common configurations
---------------------

.. code-block:: console

   # Minimal build: no MPI, no PAPI.
   python3 tests/run_tests.py --no-mpi --papi none

   # MPI build.
   python3 tests/run_tests.py --mpi --papi none

   # Test-only local PAPI installation.
   python3 tests/run_tests.py --mpi --papi local

   # PAPI counters with wallclock timing (the default).
   python3 tests/run_tests.py --mpi --papi local --test-papi-counters

   # PAPI counters without wallclock timing.
   python3 tests/run_tests.py --mpi --papi local --test-papi-counters --test-no-wallclock

   # Already installed PAPI prefix.
   python3 tests/run_tests.py --papi system --papi-prefix /path/to/papi

   # Clang C compiler with gfortran for Fortran.
   python3 tests/run_tests.py --cc clang --cxx clang++ --fc gfortran

   # Skip Fortran.
   python3 tests/run_tests.py --no-fortran

   # Exclude slow tests.
   python3 tests/run_tests.py --fast

Compile/runtime matrix
----------------------

The matrix driver builds each compile variant once and reuses it for the
different runtime counter/timing modes:

.. code-block:: console

   python3 tests/run_tests.py --matrix all --papi local

When MPI compilers are available, MPI compile variants run first, followed by
no-MPI variants. For each PAPI-enabled build, the runtime modes are:

- PAPI counters with wallclock timing
- PAPI counters without wallclock timing
- wallclock only

A PAPI-disabled build runs only the wallclock mode. At the end, the driver
prints a summary with compile options, runtime mode, pass/fail status, and
skipped/failed test details. It exits non-zero if any combination failed.

The compatibility wrapper remains available:

.. code-block:: console

   tests/run_tests_all.sh

Use ``PAPI_MODE=system tests/run_tests_all.sh`` to use a system PAPI
installation instead of the test-local one. Use the make targets for common
workflows:

.. code-block:: console

   make test          # fast default suite
   make test_papi     # PAPI-enabled core tests
   make test_hardware # PAPI hardware tests
   make test_all      # complete compile/runtime matrix

PAPI counters
-------------

To check quickly whether PAPI counters are accessible through the PerfRegions
library, run:

.. code-block:: console

   make test_papi_through_perf_regions

This configures and builds a PAPI-enabled library, then runs an instrumented
test that verifies the requested PAPI counters appear in the PerfRegions output
and have nonzero values. It fails if PAPI hardware counters are unavailable. It
uses the test-local PAPI installation by default.
To use a system PAPI instead:

.. code-block:: console

   make test_papi_through_perf_regions \
       PAPI_MODE=system PAPI_PREFIX=/path/to/papi

The counters can be changed with ``PAPI_COUNTERS``:

.. code-block:: console

   make test_papi_through_perf_regions \
       PAPI_COUNTERS=PAPI_TOT_CYC,PAPI_L3_TCM,WALLCLOCKTIME

By default the tests use wallclock timing:

.. code-block:: text

   PERF_REGIONS_COUNTERS=WALLCLOCKTIME

Run hardware-counter tests explicitly with:

.. code-block:: console

   python3 tests/run_tests.py --mpi --papi local --test-papi-counters

This requires working PAPI hardware-counter access. Many containers, virtual
machines, and CI runners do not permit hardware counters. Wallclock timing is
included by default; add ``--test-no-wallclock`` to test only the PAPI
counters. When the run uses wallclock-only tests, the runner prints the command
needed to enable hardware-counter tests at the end. Check the local
environment with:

.. code-block:: console

   source tests/tools/papi-local/env.sh
   tests/tools/papi-local/doctor.sh --strict

If hardware-counter access is unavailable, check the Linux permission setting:

.. code-block:: console

   cat /proc/sys/kernel/perf_event_paranoid

Uncore tests, including the STREAM memory-bandwidth test, require
``perf_event_paranoid`` to be ``0`` or the process to have ``CAP_PERFMON``.
A value of ``1`` may allow core counters while still blocking uncore events.

On many systems, lowering the value allows PAPI to access the counters:

.. code-block:: console

   sudo sysctl -w kernel.perf_event_paranoid=1

Use ``0`` or ``-1`` only if you understand the additional access this grants.
Containers and virtual machines may also require host-side configuration.

Test-only local PAPI
--------------------

If PAPI is not installed on the system, install a dedicated test copy under
``tests/.deps``:

.. code-block:: console

   tests/tools/papi-local/install.sh
   source tests/tools/papi-local/env.sh
   tests/tools/papi-local/doctor.sh

Then run:

.. code-block:: console

   python3 tests/run_tests.py --mpi --papi local

The runner also installs the local PAPI automatically when ``--papi local`` is
used and no installation exists yet. See ``tests/tools/papi-local/README.md``
for details and cleanup.

Running pytest directly
-----------------------

If the library has already been configured and built, pytest can be run
directly:

.. code-block:: console

   python3 -m pytest tests

The shared fixtures live in ``tests/conftest.py``. They add the build directory
to ``LD_LIBRARY_PATH``, write JSON outputs to pytest temporary directories, and
centralize the PAPI and wallclock selection.

Useful options:

.. code-block:: console

   python3 -m pytest tests -m "not slow"
   python3 -m pytest tests -m papi_hardware
   python3 -m pytest tests/functional/instrumentation/array_test_c -q

Instrumentation golden files
----------------------------

The tests verify both the instrumented source and the cleanup result against
files under ``test_data/``. For example:

- ``tests/functional/instrumentation/array_test_c/test_data/array_test.c.expected_original``
- ``tests/functional/instrumentation/array_test_c/test_data/array_test.c.expected_instrumented``

``expected_original`` is the expected original source and
``expected_instrumented`` is the expected instrumented source. When changing
the instrumentation output intentionally, update these fixtures together with
the tests.

Additional regression tests cover repeated starts of the same region ID
(``tests/functional/regions/repeated_start_c``) and dynamic region-table growth
beyond the initial allocation
(``tests/functional/regions/dynamic_regions_c``).
