# Performance Regions Library

A high-performance library for measuring and profiling different code regions, supporting both wallclock time and hardware performance counters through PAPI.

## Features

- **Region-based performance measurement**: Mark and measure specific code sections
- **Multiple measurement backends**: POSIX timers and PAPI hardware counters
- **Multi-language support**: C and Fortran APIs
- **MPI support**: Parallel performance profiling capabilities

## Dependencies

### Required
- C compiler (gcc, clang)
- Fortran compiler (gfortran, ifort)
- Python 3 (for instrumentation scripts)

### Optional
- **MPI**: For parallel applications (enabled by default)
- **PAPI**: For hardware performance counters

## Building

The library supports three build methods: autotools-style configure script (recommended for most users), modern CMake, and traditional Makefiles.

### Autotools-style Configure Script (Recommended)

The most familiar way for users accustomed to autotools:

#### Quick Start
```bash
# Configure, build and install
./configure
make
sudo make install

# Or use helper scripts
./configure
./build.sh      # Build the project
sudo ./install.sh  # Install the project
```
