Continuous integration
======================

The repository uses GitHub Actions for Linux tests. The workflow is defined in
``.github/workflows/ci.yml``.

Linux matrix
------------

The main Linux job runs on Ubuntu 24.04 with these variants:

- GCC with MPI, local test PAPI, and Fortran. This job runs the complete
  PAPI/wallclock matrix through ``tests/run_tests_all.sh``.
- GCC without MPI and without PAPI, with Fortran.
- Clang with the latest supported LLVM toolchain, without MPI and without PAPI,
  with gfortran for Fortran.

The Clang job installs LLVM from the official LLVM apt repository and installs
the matching ``libomp`` development package because the stream benchmark uses
``-fopenmp``.

Each job creates a Python virtual environment and installs the test
requirements. The PAPI job attempts to enable Linux hardware counters with
``kernel.perf_event_paranoid=1`` and then runs:

.. code-block:: console

   tests/run_tests_all.sh

The script covers no-PAPI, PAPI-wallclock-only, PAPI+wallclock, and
PAPI-counter-only runs for both no-MPI and MPI configurations. Hardware-counter
cases are skipped automatically if the runner does not permit counter access.

NVIDIA HPC SDK job
------------------

An optional, non-blocking container job exercises the NVIDIA HPC SDK compilers.
It uses the pinned NVHPC container image from the workflow and runs the test
runner with ``nvc``, ``nvc++``, and ``nvfortran``.

Documentation
-------------

GitHub Actions builds the documentation with
``.github/workflows/static.yml``. Pull requests build the Sphinx site without
publishing it; pushes to the default branch publish the generated HTML to
GitHub Pages. Enable GitHub Pages in the repository settings and select
**GitHub Actions** as the publishing source. The workflow also runs
``make -C docs linkcheck`` to check the documentation links.

To reproduce the documentation build locally:

.. code-block:: console

   python3 -m venv .venv
   source .venv/bin/activate
   python3 -m pip install -r docs/requirements.txt
   make -C docs html
