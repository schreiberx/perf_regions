Annotations
===========

PerfRegions instruments code by replacing lightweight annotations with calls to
the PerfRegions library. Annotations are supported in C and Fortran, and the
instrumentation script also recognizes NEMO-style ``timing_*`` calls in Fortran.

Native annotations
------------------

C annotations use ``#pragma perf_regions``:

.. code-block:: c

   #pragma perf_regions include

   int main(void)
   {
   #pragma perf_regions init

   #pragma perf_regions start compute
       do_work();
   #pragma perf_regions stop compute

   #pragma perf_regions finalize
       return 0;
   }

Fortran annotations use either ``!$perf_regions`` or
``!pragma perf_regions``:

.. code-block:: fortran

   PROGRAM main
   !$perf_regions include

   !$perf_regions init

   !$perf_regions start compute
       CALL do_work()
   !$perf_regions stop compute

   !$perf_regions finalize
   END PROGRAM main

Supported identifiers
---------------------

.. list-table::
   :header-rows: 1

   * - Identifier
     - Description
   * - ``include``
     - Replaced by the C header include or the Fortran ``USE`` statement.
   * - ``init``
     - Initializes the library without MPI reduction.
   * - ``init_mpi [communicator]``
     - Initializes the library and records the MPI communicator used for
       reduction at finalize time.
   * - ``finalize``
     - Finalizes the library and writes the configured output.
   * - ``start [name]``
     - Starts a named region. The name must match the corresponding ``stop``.
   * - ``stop [name]``
     - Stops a named region.
   * - ``reset``
     - Recognized for compatibility. PerfRegions has no equivalent operation and
       emits a comment into the instrumented source.

Region names and IDs
--------------------

The instrumentation script assigns one integer ID to each unique region name
across all source files processed in a single run. This is why all source files
that belong to an application should be passed to the same invocation.

Region names should be unique. The instrumentation script:

- removes surrounding whitespace,
- replaces spaces with underscores,
- removes surrounding single or double quotes.

A maximum of 128 distinct regions is supported. Reusing the same region name in
multiple places reuses the same ID.

The region name is stored by the library only on the first start of a region
ID. Later starts with the same ID reuse the stored name and do not allocate or
copy it again.

Fortran strings and region names
--------------------------------

Fortran strings are not guaranteed to be NUL-terminated. The Fortran wrapper
therefore passes the trimmed string length to the C entry point
``perf_region_start_fortran``. The C side copies the Fortran characters into a
NUL-terminated buffer on the first start of that region only.

NEMO compatibility
------------------

For Fortran code, the instrumentation script also recognizes common NEMO timing
constructs:

- ``use timing``
- ``CALL timing_init()``
- ``CALL timing_finalize()``
- ``CALL timing_start('name')``
- ``CALL timing_stop('name')``

These are replaced with the corresponding PerfRegions calls. See
:doc:`examples` for a NEMO-style example.

Nesting
-------

Nested regions are supported. When hardware counters are used, performance
counters for outer regions can be affected by inner regions or by the
measurement overhead. The output contains a ``SPOILED`` field when nested
counter measurements may be affected.
