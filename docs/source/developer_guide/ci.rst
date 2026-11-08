Continuous integration
======================

The repository uses GitHub Actions for Linux tests. The workflow is defined in
``.github/workflows/ci.yml``.

Linux matrix
------------

The main Linux job runs on Ubuntu 24.04 with these variants:

- GCC with MPI, local test PAPI, and Fortran.
- GCC without MPI and without PAPI, with Fortran.
- Clang with the latest supported LLVM toolchain, without MPI and without PAPI,
  with gfortran for Fortran.

The Clang job installs LLVM from the official LLVM apt repository and installs
the matching ``libomp`` development package because the stream benchmark uses
``-fopenmp``.

Each job creates a Python virtual environment, installs the test requirements,
and runs:

.. code-block:: console

   python3 tests/run_tests.py ...

The PAPI job also has an optional hardware-counter step. It runs only when
``tests/tools/papi-local/doctor.sh --strict`` succeeds, and is allowed to fail
without failing the workflow because free runners usually do not permit
hardware counter access.

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
