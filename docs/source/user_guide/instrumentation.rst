Instrumenting source code
=========================

The instrumentation script is ``scripts/perf_regions.py``. It supports two
modes: ``preprocess`` to inject PerfRegions calls and ``cleanup`` to restore the
original source.

Preprocess
----------

Pass one or more files or glob patterns after the mode:

.. code-block:: console

   python3 scripts/perf_regions.py preprocess 'src/*.F90'

The script processes all matching Fortran and C source files in one invocation
so that region IDs are allocated consistently across the application.

To write the instrumented files to a separate directory instead of modifying the
inputs in place:

.. code-block:: console

   python3 scripts/perf_regions.py preprocess --output-directory build 'src/*.F90'

If no targets are given, the script defaults to ``./build/*``.

During preprocessing, PerfRegions records the original line and the injected
line using marker comments:

- Fortran: ``!PERF_REGION_ORIGINAL`` and ``!PERF_REGION_CODE``
- C: ``//PERF_REGION_ORIGINAL`` and ``//PERF_REGION_CODE``

The script stops with an error if an input file already contains these markers.
Run the cleanup mode first in that case.

Cleanup
-------

Cleanup restores the original source from the marker comments:

.. code-block:: console

   python3 scripts/perf_regions.py cleanup 'build/*.F90'

The cleanup mode can be used both for files that were modified in place and for
instrumented copies in a build directory.

Validation performed by the script
----------------------------------

The preprocessing run fails if:

- an unknown file extension is encountered,
- a ``stop`` annotation has no matching ``start``,
- no initialization annotation is present,
- no finalization annotation is present,
- an input file still contains previous instrumentation markers.

The script prints the final region-name list, including the assigned IDs.

Test integration
----------------

The test suite uses the same instrumentation logic through
``tests/tools/instrument.py``. See :doc:`../developer_guide/testing` for the
test runner and how the instrumentation output is verified against golden files.
