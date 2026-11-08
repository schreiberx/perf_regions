Environment variables
=====================

PerfRegions reads the following environment variables during
``perf_regions_init()``.

.. list-table::
   :header-rows: 1

   * - Variable
     - Default
     - Description
   * - ``PERF_REGIONS_COUNTERS``
     - unset
     - Comma-separated list of PAPI event names, memory-transfer pseudo events
       (``PR_BANDWIDTH_BYTES``, ``PR_BANDWIDTH_GB``,
       ``PR_MEM_TRANSFER_BYTES``, ``PR_MEM_TRANSFER_GB``), and/or
       ``WALLCLOCKTIME``. If unset, wallclock timing is enabled. If set,
       wallclock timing is disabled unless ``WALLCLOCKTIME`` is included.
   * - ``PERF_REGIONS_SKIP_N``
     - ``0``
     - Number of initial executions of each region to skip. Negative values are
       clamped to zero.
   * - ``PERF_REGIONS_VERBOSITY``
     - ``0``
     - Integer verbosity level for initialization diagnostics.
   * - ``PERF_REGIONS_OUTPUT``
     - console output
     - Comma-separated output configuration. Supported tokens are ``console``,
       ``json``, and ``json=<filename>``. Setting the variable disables the
       default console output unless ``console`` is included.
   * - ``PERF_REGIONS_BANDWIDTH_EVENTS``
     - unset
     - Explicit comma-separated PAPI event list for the memory-transfer pseudo
       events. Skips automatic memory-channel detection.
   * - ``PERF_REGIONS_BANDWIDTH_BYTES_PER_COUNT``
     - ``64``
     - Scale factor used with ``PERF_REGIONS_BANDWIDTH_EVENTS``.
   * - ``PERF_REGIONS_BANDWIDTH_CPU``
     - auto-detected
     - CPU qualifier used for uncore memory events.
   * - ``PERF_REGIONS_BANDWIDTH_PROFILE``
     - auto-detected
     - Force a memory-event profile such as ``intel-imc``, ``intel-qmc``,
       ``amd-df``, ``arm-l2``, or ``power8``.
   * - ``LD_LIBRARY_PATH``
     - system default
     - Runtime search path for the PerfRegions shared library when using an
       uninstalled build or a non-standard installation prefix.

Examples
--------

Wallclock only:

.. code-block:: console

   export PERF_REGIONS_COUNTERS=WALLCLOCKTIME
   ./application

PAPI counters and wallclock time:

.. code-block:: console

   export PERF_REGIONS_COUNTERS=PAPI_TOT_CYC,PAPI_L3_TCM,WALLCLOCKTIME
   ./application

Automatically detected memory-transfer volume:

.. code-block:: console

   export PERF_REGIONS_COUNTERS=PR_BANDWIDTH_BYTES,PR_BANDWIDTH_GB,PR_MEM_TRANSFER_BYTES,PR_MEM_TRANSFER_GB
   ./application

Skip the first two executions of each region:

.. code-block:: console

   export PERF_REGIONS_SKIP_N=2
   ./application

Human-readable console output and JSON file:

.. code-block:: console

   export PERF_REGIONS_OUTPUT=console,json=perf_regions.json
   ./application

See :doc:`../user_guide/running` for usage examples and
:doc:`../user_guide/output` for the output fields.
