Installation
============

Dependencies
------------

Required tools:

- A C compiler such as GCC or Clang.
- ``make`` and ``ar``.
- Python 3 for the instrumentation script.

Optional components:

- A Fortran compiler such as ``gfortran``, ``ifort``, or ``nvfortran`` for the
  Fortran interface.
- MPI compilers such as ``mpicc`` and ``mpif90`` for MPI support.
- PAPI for hardware performance counters.

PerfRegions can be built without MPI, without PAPI, and without Fortran
support. The configuration script auto-detects available components and disables
the ones it cannot find unless they were explicitly requested.

Configure and build
-------------------

PerfRegions uses an autotools-style configuration script:

.. code-block:: console

   ./configure
   make -j
   sudo make install

Run the following command to list all configuration options:

.. code-block:: console

   ./configure --help

Common options:

- ``--prefix=DIR`` sets the installation prefix. The default is
  ``/usr/local``.
- ``--enable-mpi`` / ``--disable-mpi`` enables or disables MPI support.
- ``--enable-papi`` / ``--disable-papi`` enables or disables PAPI support.
- ``--with-papi=DIR`` points to a PAPI installation.
- ``--with-mpi=DIR`` points to an MPI installation.
- ``--enable-fortran`` / ``--disable-fortran`` enables or disables the Fortran
  interface.
- ``--enable-shared`` / ``--disable-shared`` selects shared-library builds.
  Disabling shared libraries enables static libraries.
- ``--enable-static`` / ``--disable-static`` selects static-library builds.
- ``--enable-debug`` builds with debugging information and ``-O0``.

The configuration is written to ``config.mk``. The top-level Makefile also
supports direct builds, for example:

.. code-block:: console

   make USE_MPI=0 USE_PAPI=0 USE_FORTRAN=1
   make install PREFIX=/path/to/prefix

Staged installation is supported through ``DESTDIR``:

.. code-block:: console

   make install PREFIX=/usr DESTDIR=/tmp/stage

Library names
-------------

The library name records which optional features are enabled. The suffixes are
appended in this order:

- ``_papi`` when PAPI support is enabled,
- ``_mpi`` when MPI support is enabled,
- ``_fort`` when the Fortran interface is enabled.

Examples:

- ``libperf_regions.so``: C API, no MPI, no PAPI.
- ``libperf_regions_mpi.so``: C API with MPI.
- ``libperf_regions_papi_mpi_fort.so``: PAPI, MPI, and Fortran interface.

Use the same feature combination in the application build as in the library
build.

Compile and link flags
----------------------

For an installed prefix, add the public headers to the include path:

.. code-block:: console

   -I<prefix>/include

For Fortran code, the same include path contains the compiled module file.

For C applications without MPI or PAPI:

.. code-block:: console

   -L<prefix>/lib -lperf_regions

For a Fortran-enabled build, link against the Fortran library:

.. code-block:: console

   -L<prefix>/lib -lperf_regions_fort

Add the feature suffixes as needed. For a PAPI-enabled build, also add the PAPI
library and, if it is installed outside the default search path, its library
directory:

.. code-block:: console

   -L<prefix>/lib -lperf_regions_papi -lpapi

When using an uninstalled build, point the compiler at ``<repository>/src``
and the matching variant directory, for example
``<repository>/build_papi_mpi_fort``, instead of the installed prefix.

Runtime library path
--------------------

Installed libraries use an RPATH pointing to the configured library directory.
For an uninstalled development build, add the matching variant build directory
to ``LD_LIBRARY_PATH``:

.. code-block:: console

   export LD_LIBRARY_PATH="$PWD/build_papi_mpi_fort:$LD_LIBRARY_PATH"

See :doc:`running` for the runtime environment variables.
