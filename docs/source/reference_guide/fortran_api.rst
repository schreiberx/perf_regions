Fortran API
===========

The Fortran interface is provided by the module ``perf_regions_fortran``. Add:

.. code-block:: fortran

   USE perf_regions_fortran

The module declares C-interoperable interfaces and provides a Fortran wrapper
for starting regions with character strings.

Public procedures
-----------------

``CALL perf_regions_init()``
   Initialize the library. This is equivalent to the C
   ``perf_regions_init()``.

``CALL perf_regions_init_mpi_fortran(communicator)``
   Initialize the library and register an MPI communicator. ``communicator`` is
   a Fortran MPI communicator handle.

``CALL perf_region_start(region_id, region_name)``
   Start a region.

   ``region_id`` is an integer region ID. ``region_name`` is a Fortran
   character string. The wrapper computes the trimmed length and forwards the
   character data and length to the C entry point. The string is copied by the
   library only on the first start of that region ID.

``CALL perf_region_stop(region_id)``
   Stop a region.

``CALL perf_regions_finalize()``
   Finalize the library and write the configured output.

Example
-------

.. code-block:: fortran

   PROGRAM main
   USE perf_regions_fortran
   IMPLICIT NONE

   CALL perf_regions_init()

   CALL perf_region_start(0, "compute")
   CALL do_work()
   CALL perf_region_stop(0)

   CALL perf_regions_finalize()
   END PROGRAM main

String handling
---------------

Fortran character strings are not guaranteed to be NUL-terminated. The Fortran
wrapper therefore passes the trimmed length as a ``C_SIZE_T`` value. The C
implementation stores a NUL-terminated copy on the first start of the region ID
and reuses that copy on all later starts.

This means repeated calls such as:

.. code-block:: fortran

   CALL perf_region_start(0, "compute")
   CALL perf_region_stop(0)
   CALL perf_region_start(0, "compute")
   CALL perf_region_stop(0)

do not allocate or copy the region name on the second start.

MPI communicator
----------------

The communicator argument of ``perf_regions_init_mpi_fortran`` is converted to
a C communicator with ``MPI_Comm_f2c``. The library must be built with MPI
support.

See :doc:`../user_guide/mpi` for the annotation-based initialization form.
