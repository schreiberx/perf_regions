PerfRegions documentation
=========================

PerfRegions is a library and source-instrumentation tool for measuring selected
regions of C and Fortran applications. It combines wallclock timing with
optional PAPI hardware performance counters and supports MPI reductions over
the measured results.

The recommended workflow is to mark code regions with lightweight annotations,
run the PerfRegions instrumentation script once over all source files, and then
build the application against the PerfRegions library.

Features
--------

- Region-based wallclock timing.
- Optional PAPI hardware performance counters.
- Optional MPI support with accumulation over communicator ranks.
- C and Fortran APIs.
- Native ``perf_regions`` annotations and NEMO ``timing_*`` compatibility.
- Human-readable console output and machine-readable JSON output.
- Runtime selection of counters, skipped executions, verbosity, and output
  destination through environment variables.
- Automatic source restoration through the instrumentation cleanup mode.

Quick start
-----------

From the repository root:

.. code-block:: console

   ./configure --enable-mpi --enable-papi
   make -j
   sudo make install

If PAPI is not needed, disable it explicitly:

.. code-block:: console

   ./configure --disable-mpi --disable-papi --enable-fortran
   make -j

See :doc:`user_guide/installation` for detailed build, link, and installation
instructions.

Annotating a region in C
------------------------

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

Annotating a region in Fortran
------------------------------

.. code-block:: fortran

   PROGRAM main
   !$perf_regions include

   !$perf_regions init

   !$perf_regions start compute
       CALL do_work()
   !$perf_regions stop compute

   !$perf_regions finalize
   END PROGRAM main

.. toctree::
   :maxdepth: 2
   :caption: Documentation:

   User Guide <user_guide/index>
   Developer Guide <developer_guide/index>
   Reference Guide <reference_guide/index>
   About <about>
