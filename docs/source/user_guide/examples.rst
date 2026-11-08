Examples
========

The repository contains complete examples under ``tests/``. The snippets below
show the minimal annotation and instrumentation workflow.

Minimal C example
-----------------

Original source:

.. code-block:: c

   #include <stdio.h>

   #pragma perf_regions include

   static void do_work(void)
   {
       volatile double value = 0.0;
       for (int i = 0; i < 100000; ++i)
           value += (double)i;
   }

   int main(void)
   {
   #pragma perf_regions init

       for (int i = 0; i < 10; ++i)
       {
   #pragma perf_regions start compute
           do_work();
   #pragma perf_regions stop compute
       }

   #pragma perf_regions finalize
       return 0;
   }

Run the instrumentation script:

.. code-block:: console

   python3 scripts/perf_regions.py preprocess example.c

The generated call sequence is equivalent to:

.. code-block:: c

   #include <perf_regions.h>

   perf_regions_init();

   perf_region_start(0, "compute");
   do_work();
   perf_region_stop(0);

   perf_regions_finalize();

Minimal Fortran example
-----------------------

Original source:

.. code-block:: fortran

   PROGRAM main
   !$perf_regions include

   !$perf_regions init

       DO i = 1, 10
   !$perf_regions start compute
           CALL do_work()
   !$perf_regions stop compute
       END DO

   !$perf_regions finalize
   END PROGRAM main

After instrumentation:

.. code-block:: fortran

   USE perf_regions_fortran

   CALL perf_regions_init()

   CALL perf_region_start(0, "compute")
   CALL do_work()
   CALL perf_region_stop(0)   ! name="compute"

   CALL perf_regions_finalize()

NEMO-style timing calls
-----------------------

The instrumentation script also recognizes NEMO-style calls in Fortran:

.. code-block:: fortran

   SUBROUTINE test1
       CALL timing_start('FOOa')
       CALL timing_start('FOOb')
       CALL test2
       CALL timing_stop('FOOb')
       CALL timing_stop('FOOa')
   END SUBROUTINE test1

These calls are replaced with PerfRegions region starts and stops while keeping
the original nesting order.

Repository examples
-------------------

See the test sources for more complete programs:

- ``tests/functional/instrumentation/array_test_c``: C annotations and instrumentation.
- ``tests/functional/instrumentation/array_test_f``: Fortran annotations and instrumentation.
- ``tests/functional/instrumentation/nemo_test_f`` and
  ``tests/functional/instrumentation/nemo_test_f_direct``: NEMO-style timing
  calls.
- ``tests/benchmarks/stream_benchmark``: a larger OpenMP benchmark
  instrumented with PerfRegions.
- ``tests/hardware/papi``: direct PAPI and PerfRegions PAPI counter checks.

See :doc:`../developer_guide/testing` for running the complete test suite.
