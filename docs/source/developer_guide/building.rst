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

Each feature combination gets its own build directory. The directory name
uses the same suffix as the library:

- ``build/build/`` for the base C build,
- ``build/build_papi/`` for PAPI,
- ``build/build_mpi/`` for MPI,
- ``build/build_fort/`` for Fortran,
- ``build/build_papi_mpi_fort/`` for PAPI, MPI, and Fortran.

The generated configuration header is stored in the matching build directory,
for example ``build/build_papi_mpi_fort/perf_regions_config.h``, and is
installed together with the public headers. The directory can be overridden
with ``BUILD_DIR=...``.

The library suffix depends on the enabled features:

- ``_papi`` for PAPI,
- ``_mpi`` for MPI,
- ``_fort`` for Fortran.

For example, ``build/build_papi_mpi_fort/libperf_regions_papi_mpi_fort.so``
is a PAPI, MPI, and Fortran build.

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

The test runner builds and cleans only the variants used by the current
invocation and reuses one build for multiple runtime modes. Running
``make clean`` removes the complete ``build/`` tree. Use ``--keep-build`` to
retain builds for debugging.

Documentation
-------------

The Sphinx documentation lives in ``docs/``. The installer creates a local
virtual environment, installs the documentation requirements, and builds the
HTML:

.. code-block:: console

   docs/install.sh

The generated HTML is written to ``docs/build/html``.

To install the dependencies without building:

.. code-block:: console

   docs/install.sh --no-build

To clean the generated documentation before rebuilding:

.. code-block:: console

   docs/install.sh --clean

The virtual environment directory can be changed with ``--venv DIR`` or
``DOCS_VENV_DIR``. Additional Sphinx options can be passed through
``SPHINXOPTS``.
