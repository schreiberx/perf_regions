MPI support
===========

When PerfRegions is built with MPI support, measurements can be accumulated
across the ranks of an MPI communicator. The reduction is performed at
finalize time and rank 0 writes the combined results.

Initialization
--------------

Use the MPI initializer instead of the plain initializer.

In C:

.. code-block:: c

   perf_regions_init_mpi(MPI_COMM_WORLD);

With annotations:

.. code-block:: c

   #pragma perf_regions init_mpi MPI_COMM_WORLD

In Fortran:

.. code-block:: fortran

   CALL perf_regions_init_mpi_fortran(MPI_COMM_WORLD)

With annotations:

.. code-block:: fortran

   !$perf_regions init_mpi MPI_COMM_WORLD

The communicator expression is forwarded to the C entry point. The Fortran
wrapper converts the Fortran communicator with ``MPI_Comm_f2c``.

Reduction semantics
-------------------

At finalize time, PerfRegions:

- gathers sample counts, wallclock statistics, skipped counts, and spoilage
  flags from all ranks,
- combines the wallclock statistics into global values,
- sums PAPI counter values across ranks,
- writes the combined result from rank 0.

The reduction is collective for every region ID. A rank that never entered a
region participates with zero samples, so regions executed only on a subset of
ranks do not cause a deadlock.

Output
------

Both human-readable and JSON output use the combined values when MPI reduction
is active. Only rank 0 writes the output.

For an example, a human-readable wallclock result contains the accumulated total
time, global minimum, maximum, mean, and variance over the valid samples from all
ranks.

Building and linking
--------------------

Use a library built with MPI support, for example
``libperf_regions_mpi.so``. Compile and link through the MPI compiler wrappers
or add the MPI include and link flags required by the MPI implementation.

See :doc:`installation` for the feature-suffix naming scheme.
