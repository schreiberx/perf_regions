Building from a development checkout
====================================

The repository supports an autotools-style configuration step and direct
invocations of the top-level Makefile.

Configure-based build
---------------------

.. code-block:: console

   ./configure --enable-fortran
   make -j"$(nproc)"

The configuration script writes ``config.mk``. The top-level Makefile includes
that file and forwards the selected options to ``src/Makefile`` and the test
Makefiles.

Direct Makefile build
---------------------

Configuration is optional. The Makefile defaults can be overridden on the
command line or through environment variables:

.. code-block:: console

   make USE_MPI=0 USE_PAPI=0 USE_FORTRAN=1

Important variables:

- ``USE_MPI=0|1``
- ``USE_PAPI=0|1``
- ``USE_FORTRAN=0|1``
- ``BUILD_SHARED_LIBS=0|1``
- ``BUILD_STATIC_LIBS=0|1``
- ``PREFIX``
- ``DESTDIR``

If PAPI is enabled, provide ``PAPI_CFLAGS`` and ``PAPI_LDFLAGS`` or configure
with ``--with-papi=DIR``.

Build outputs
-------------

The library is written to ``build/``. The generated configuration header is
``build/perf_regions_config.h`` and is installed together with the public
headers.

The library suffix depends on the enabled features:

- ``_papi`` for PAPI,
- ``_mpi`` for MPI,
- ``_fort`` for Fortran.

For example, ``libperf_regions_papi_mpi_fort.so`` is a PAPI, MPI, and Fortran
build.

Useful Make targets
-------------------

.. list-table::
   :header-rows: 1

   * - Target
     - Description
   * - ``all`` / ``build_src``
     - Build the library.
   * - ``tests`` / ``run_tests``
     - Run the test suite through the tests Makefile.
   * - ``install``
     - Install libraries, headers, the Fortran module, and the instrumentation
       script.
   * - ``uninstall``
     - Remove installed files.
   * - ``clean``
     - Remove build artifacts.
   * - ``distclean``
     - Remove generated configuration files and log files as well.

Cleaning
--------

.. code-block:: console

   make clean
   make distclean

The test runner normally cleans the build automatically before and after a run.
Use ``--keep-build`` to retain the build for debugging.

Documentation
-------------

The Sphinx documentation lives in ``docs/``:

.. code-block:: console

   python3 -m venv .venv
   source .venv/bin/activate
   python3 -m pip install -r docs/requirements.txt
   make -C docs html

The generated HTML is written to ``docs/build/html``.
