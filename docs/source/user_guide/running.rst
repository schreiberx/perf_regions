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
     - Comma-separated list of PAPI event names and/or ``WALLCLOCKTIME``.
       If unset, PerfRegions measures wallclock time only. If set, wallclock
       timing is disabled unless ``WALLCLOCKTIME`` is included explicitly.
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

For an uninstalled build, set ``LD_LIBRARY_PATH`` before running:

.. code-block:: console

   export LD_LIBRARY_PATH="$PWD/build:$LD_LIBRARY_PATH"
   ./my_application
