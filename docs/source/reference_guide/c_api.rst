C API
=====

The public C interface is declared in ``perf_regions.h``. The internal layout of
``struct PerfRegions`` is private and may depend on the library's build options.

Region IDs
----------

Region IDs are non-negative integers. The region table starts with
``PERF_REGIONS_MIN`` slots (currently 128) and doubles its capacity whenever a
larger ID is used. Negative IDs are treated as fatal errors.

.. c:function:: void perf_regions_init(void)

   Initialize the library and read the runtime configuration from the
   environment.

   This function is idempotent. Calling it again after a successful
   initialization is ignored.

.. c:function:: void perf_regions_init_mpi(MPI_Comm communicator)

   Initialize the library and register an MPI communicator for result
   reduction at finalize time.

   This function is only available in builds with MPI support.

.. c:function:: void perf_regions_init_mpi_fortran(int communicator)

   Fortran-facing MPI initializer. The Fortran communicator handle is converted
   with ``MPI_Comm_f2c`` and passed to :c:func:`perf_regions_init_mpi`.

   This function is only available in builds with MPI support.

.. c:function:: void perf_region_start(int i_region_id, const char *i_region_name)

   Start measuring the region identified by ``i_region_id``.

   The region name is stored on the first start of the ID. Later starts with
   the same ID reuse the stored name and do not copy it again. The name must be
   non-``NULL`` on the first start.

   Nested regions are supported. The number of simultaneously active nested
   regions is limited by ``PERF_NESTED_REGIONS_MAX``.

.. c:function:: void perf_region_start_fortran(int i_region_id, const char *i_region_name, size_t len)

   Fortran-facing region start. ``i_region_name`` points to the Fortran
   character data and ``len`` gives the number of characters to use.

   On the first start of the region ID, the function allocates a
   NUL-terminated buffer of ``len + 1`` characters, copies exactly ``len``
   characters, and appends the terminator. Subsequent starts with the same ID
   do not allocate or copy the name again.

   This entry point is normally called through the ``perf_region_start``
   wrapper in ``perf_regions_fortran``.

.. c:function:: void perf_region_stop(int i_region_id)

   Stop measuring the region identified by ``i_region_id``.

   A stop call must match the most recent start of the same region in the
   nesting order.

.. c:function:: void perf_regions_output_human_readable_text(void)

   Write the human-readable result table to standard output.

   This function is normally called indirectly through
   :c:func:`perf_regions_finalize` when console output is enabled.

.. c:function:: void perf_regions_finalize(void)

   Finalize the library, optionally perform MPI reduction, write the configured
   output, and release allocated resources.

   This function is idempotent. Calling it when the library is not initialized
   has no effect.

   Calling finalize while regions are still active produces a diagnostic on
   standard error.

Notes
-----

- ``PERF_REGIONS_MIN`` and ``PERF_NESTED_REGIONS_MAX`` are defined in
  ``perf_regions_defines.h``.
- The library stores process-global state and does not use internal locking.
- All public headers are installed together with the generated
  ``perf_regions_config.h`` that records the build configuration.
