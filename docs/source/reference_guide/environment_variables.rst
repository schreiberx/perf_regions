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
     - Comma-separated list of PAPI event names and/or ``WALLCLOCKTIME``. If
       unset, wallclock timing is enabled. If set, wallclock timing is disabled
       unless ``WALLCLOCKTIME`` is included.
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
