Output format
=============

PerfRegions writes results when ``perf_regions_finalize()`` is called. The
available columns and fields depend on the build options and the counters
selected at runtime.

Human-readable output
---------------------

Console output is enabled by default. Each line starts with the
``[MULE] perf_regions:`` prefix so that it can be filtered easily.

Example:

.. code-block:: text

   [MULE] perf_regions: Performance counters profiling:
   [MULE] perf_regions: ----------------------
   [MULE] perf_regions: Section    PAPI_TOT_CYC   SPOILED   WALLCLOCKTIME   MIN            MAX            MEAN           VAR            COUNTER   SKIPPED
   [MULE] perf_regions: compute    1.2345678e+08  0         2.3456789e-03   2.3000000e-03  2.4000000e-03  2.3456789e-03  1.2345678e-09  10        0

The columns are:

- ``Section``: the stored region name.
- One column per requested PAPI counter.
- ``PR_BANDWIDTH_BYTES``, ``PR_BANDWIDTH_GB``,
  ``PR_MEM_TRANSFER_BYTES``, ``PR_MEM_TRANSFER_GB``: consolidated transfer
  volumes for the memory-transfer pseudo events. These columns are present
  when any of the pseudo events is requested.
- ``SPOILED``: set when nested counter measurements may be affected.
- ``WALLCLOCKTIME``: total accumulated wallclock time.
- ``MIN``, ``MAX``, ``MEAN``, ``VAR``: statistics over valid wallclock
  samples.
- ``COUNTER``: number of valid samples.
- ``SKIPPED``: number of starts skipped through ``PERF_REGIONS_SKIP_N``.

The PAPI counter columns, ``SPOILED``, and wallclock columns are only present
when they are enabled for the build and requested at runtime.

JSON output
-----------

JSON output is an array with one object per region. Example:

.. code-block:: json

   [
     {
       "region": "compute",
       "counter": 10,
       "skipped": 0,
       "wallclock_time": 2.3456789e-03,
       "min": 2.3000000e-03,
       "max": 2.4000000e-03,
       "mean": 2.3456789e-03,
       "variance": 1.2345678e-09,
       "spoiled": 0,
       "counters": {
         "PAPI_TOT_CYC": 1.2345678e+08
       }
     }
   ]

Fields:

- ``region``: the stored region name.
- ``counter``: number of valid samples.
- ``skipped``: number of skipped starts.
- ``wallclock_time``: total accumulated wallclock time.
- ``min``, ``max``, ``mean``, ``variance``: wallclock statistics.
- ``spoiled``: whether nested measurements may have affected the counters.
- ``counters``: mapping from PAPI event names to accumulated values. When a
  memory-transfer pseudo event is requested, this object also contains
  ``PR_BANDWIDTH_BYTES``, ``PR_BANDWIDTH_GB``, ``PR_MEM_TRANSFER_BYTES``, and
  ``PR_MEM_TRANSFER_GB``.

Wallclock-related fields are only emitted when wallclock timing is enabled.
The ``counters`` object is only emitted for a PAPI-enabled build with requested
PAPI events.

MPI output
----------

With MPI enabled and an MPI communicator registered at initialization, the
finalize step reduces the measurements across all ranks and rank 0 writes the
combined result. See :doc:`mpi`.
