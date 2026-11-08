User Guide
==========

This guide describes how to install PerfRegions, annotate or instrument an
application, run it, and interpret the results.

A typical workflow is:

1. Install the library and make its headers and Fortran module available.
2. Add PerfRegions annotations to the source files, or reuse supported NEMO
   ``timing_*`` calls.
3. Run the instrumentation script over all source files in one invocation.
4. Build the instrumented application and link it against the matching
   PerfRegions library.
5. Select counters and output options through environment variables.
6. Run the application and inspect the console or JSON results.

.. toctree::
   :maxdepth: 1
   :caption: Contents:

   installation
   annotations
   instrumentation
   running
   output
   mpi
   examples
