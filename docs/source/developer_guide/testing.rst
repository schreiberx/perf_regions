Testing
=======

The canonical test entry point is:

.. code-block:: console

   python3 tests/run_tests.py

The runner configures the library, builds it, runs pytest, and cleans up
afterwards. By default it uses wallclock timing, no MPI, and builds both the C
and Fortran libraries when a Fortran compiler is available.

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

   # Already installed PAPI prefix.
   python3 tests/run_tests.py --papi system --papi-prefix /path/to/papi

   # Clang C compiler with gfortran for Fortran.
   python3 tests/run_tests.py --cc clang --cxx clang++ --fc gfortran

   # Skip Fortran.
   python3 tests/run_tests.py --no-fortran

   # Exclude slow tests.
   python3 tests/run_tests.py --fast

PAPI counters
-------------

By default the tests use wallclock timing:

.. code-block:: text

   PERF_REGIONS_COUNTERS=WALLCLOCKTIME

Run hardware-counter tests explicitly with:

.. code-block:: console

   python3 tests/run_tests.py --mpi --papi local --papi-hardware

This requires working PAPI hardware-counter access. Many containers, virtual
machines, and CI runners do not permit hardware counters. Check the local
environment with:

.. code-block:: console

   source tests/tools/papi-local/env.sh
   tests/tools/papi-local/doctor.sh --strict

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
   python3 -m pytest tests/array_test_c -q

Instrumentation golden files
----------------------------

The tests verify both the instrumented source and the cleanup result against
files under ``test_data/``. For example:

- ``tests/array_test_c/test_data/array_test.c_TEST_ORIG``
- ``tests/array_test_c/test_data/array_test.c_TEST_PR``

``TEST_ORIG`` is the expected original source and ``TEST_PR`` is the expected
instrumented source. When changing the instrumentation output intentionally,
update these fixtures together with the tests.

Legacy wrapper
--------------

``tests/run_build_and_pytests.sh`` is kept as a compatibility wrapper. New usage
should call ``tests/run_tests.py`` directly.
