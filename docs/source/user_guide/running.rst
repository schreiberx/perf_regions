Running an instrumented application
===================================

PerfRegions reads its runtime configuration when ``perf_regions_init()`` is
called. Set environment variables before starting the application.

Environment variables
---------------------

.. list-table::
   :header-rows: 1

   * - Variable
     - Description
   * - ``PERF_REGIONS_COUNTERS``
     - Comma-separated list of PAPI event names, memory-transfer pseudo events,
       and/or ``WALLCLOCKTIME``. If unset, PerfRegions measures wallclock time
       only. If set, wallclock timing is disabled unless ``WALLCLOCKTIME`` is
       included explicitly.
   * - ``PERF_REGIONS_SKIP_N``
     - Number of initial executions of each region to skip. The default is
       ``0``. Skipped executions are counted separately.
   * - ``PERF_REGIONS_VERBOSITY``
     - Integer verbosity level. The default is ``0``. Higher values print
       configuration details during initialization.
   * - ``PERF_REGIONS_OUTPUT``
     - Comma-separated output destinations. Supported tokens are ``console``,
       ``json``, and ``json=<filename>``. If the variable is set, console output
       is disabled unless ``console`` is also listed. ``json`` without a
       filename writes JSON to standard output.
   * - ``LD_LIBRARY_PATH``
     - Runtime search path for the PerfRegions library when using an
       uninstalled development build or a non-standard installation prefix.

PAPI counters
-------------

PAPI event names depend on the platform. Use PAPI's inspection tools to find
available events:

.. code-block:: console

   papi_avail
   papi_native_avail

Example: measure wallclock time only:

.. code-block:: console

   export PERF_REGIONS_COUNTERS=WALLCLOCKTIME
   ./my_application

Example: measure several PAPI counters and wallclock time:

.. code-block:: console

   export PERF_REGIONS_COUNTERS=PAPI_TOT_CYC,PAPI_L3_TCM,WALLCLOCKTIME
   ./my_application

If the library was built without PAPI and the environment requests a PAPI
counter, initialization fails with an error instead of silently ignoring the
request.

Memory-transfer pseudo events
-----------------------------

The pseudo events ``PR_BANDWIDTH_BYTES``, ``PR_BANDWIDTH_GB``,
``PR_MEM_TRANSFER_BYTES``, and ``PR_MEM_TRANSFER_GB`` automatically detect the
memory-channel events available on the system. The individual channel events
are added to the output, together with all four consolidated values.

``PR_BANDWIDTH_BYTES`` and ``PR_MEM_TRANSFER_BYTES`` report the total number of
bytes transferred in the region. ``PR_BANDWIDTH_GB`` and ``PR_MEM_TRANSFER_GB``
report the same volume in decimal gigabytes. These are transfer volumes, not
throughput rates; add ``WALLCLOCKTIME`` if timing information is also needed.

.. code-block:: console

   export PERF_REGIONS_COUNTERS=PR_BANDWIDTH_BYTES,PR_BANDWIDTH_GB,WALLCLOCKTIME
   ./my_application

The following optional variables control auto-detection:

.. list-table::
   :header-rows: 1

   * - Variable
     - Description
   * - ``PERF_REGIONS_BANDWIDTH_EVENTS``
     - Explicit comma-separated PAPI event list. Skips auto-detection.
   * - ``PERF_REGIONS_BANDWIDTH_BYTES_PER_COUNT``
     - Scale factor used with explicit events. The default is ``64``.
   * - ``PERF_REGIONS_BANDWIDTH_CPU``
     - CPU qualifier to use for uncore events instead of the detected CPUs.
   * - ``PERF_REGIONS_BANDWIDTH_PROFILE``
     - Force a profile such as ``intel-imc``, ``intel-qmc``, ``amd-df``,
       ``arm-l2``, or ``power8``.

For example, to use explicit events on a system where auto-detection is not
supported:

.. code-block:: console

   export PERF_REGIONS_BANDWIDTH_EVENTS='skx_unc_imc0::UNC_M_CAS_COUNT:RD:cpu=0,skx_unc_imc0::UNC_M_CAS_COUNT:WR:cpu=0'
   export PERF_REGIONS_BANDWIDTH_BYTES_PER_COUNT=64
   export PERF_REGIONS_COUNTERS=PR_MEM_TRANSFER_BYTES,PR_MEM_TRANSFER_GB
   ./my_application

Skipping warm-up executions
---------------------------

Use ``PERF_REGIONS_SKIP_N`` to exclude the first executions of every region:

.. code-block:: console

   export PERF_REGIONS_SKIP_N=2
   ./my_application

The skipped executions still increment a skipped counter but do not contribute
to the measured values.

Console and JSON output
-----------------------

The default is human-readable console output at finalize time:

.. code-block:: console

   export PERF_REGIONS_OUTPUT=console
   ./my_application

Write JSON to a file:

.. code-block:: console

   export PERF_REGIONS_OUTPUT=json=perf_regions.json
   ./my_application

Write JSON to standard output without console output:

.. code-block:: console

   export PERF_REGIONS_OUTPUT=json
   ./my_application

Write both console output and JSON to a file:

.. code-block:: console

   export PERF_REGIONS_OUTPUT=console,json=perf_regions.json
   ./my_application

See :doc:`output` for the format of the results.

Runtime library path
--------------------

For an uninstalled build, set ``LD_LIBRARY_PATH`` to the matching variant
directory before running:

.. code-block:: console

   export LD_LIBRARY_PATH="$PWD/build/build_papi_mpi_fort:$LD_LIBRARY_PATH"
   ./my_application
