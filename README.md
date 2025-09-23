# Performance Regions Library

A high-performance library for measuring and profiling different code regions, supporting both wallclock time and hardware performance counters through PAPI.

## Features

- **Region-based performance measurement**: Mark and measure specific code sections
- **Multiple measurement backends**: POSIX timers and PAPI hardware counters
- **Multi-language support**: C and Fortran APIs
- **MPI support**: Parallel performance profiling capabilities
- **Flexible output formats**: Human-readable text and CSV output
- **Modern build system**: Autotools-style configure script, CMake, and traditional Makefiles

## Dependencies

### Required
- C compiler (gcc, clang)
- Fortran compiler (gfortran, ifort)
- Python 3 (for instrumentation scripts)

### Optional
- **MPI**: For parallel applications (enabled by default)
- **PAPI**: For hardware performance counters
- **CMake 3.16+**: For modern build system

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

#### Configuration Options
```bash
# Configure with custom prefix
./configure --prefix=/usr/local

# Enable/disable features
./configure --enable-papi --disable-mpi
./configure --enable-debug --disable-examples

# Specify library locations
./configure --with-papi=/opt/papi --with-mpi=/opt/openmpi

# Get help
./configure --help
```

#### Environment Variables
```bash
# Use specific compilers
CC=clang FC=flang ./configure

# Set compiler flags
CFLAGS="-O3 -march=native" ./configure
```

### Modern CMake Build

For developers who prefer direct CMake usage:

#### Quick Start
```bash
# Configure and build with default settings
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

#### Build Options
```bash
# Enable PAPI support
cmake -DPERF_REGIONS_USE_PAPI=ON ..

# Disable MPI support
cmake -DPERF_REGIONS_USE_MPI=OFF ..

# Build without examples
cmake -DPERF_REGIONS_BUILD_EXAMPLES=OFF ..

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

#### Using CMake Presets (CMake 3.20+)
```bash
# List available presets
cmake --list-presets

# Configure with preset
cmake --preset=default

# Build with preset
cmake --build --preset=default

# Test with preset
ctest --preset=default
```

#### Installation
```bash
# Install to system directories
sudo make install

# Install to custom location
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install ..
make install
```

### Traditional Makefile Build

For legacy compatibility:

#### Default Build
```bash
# Build library
make

# Build examples
make examples

# Run tests
make run_tests

# Debug build
make MODE=debug
```

#### Using CMake through Makefile
```bash
# Use CMake backend
make BUILD_METHOD=cmake

# Configure CMake options
make BUILD_METHOD=cmake CMAKE_OPTIONS="-DPERF_REGIONS_USE_PAPI=ON"

# Run tests with CMake
make BUILD_METHOD=cmake run_tests
```

## Usage

### C API Example
```c
#include "perf_regions.h"

int main() {
    // Initialize the library
    perf_regions_init();
    
    // Start measuring region 1
    perf_region_start(1, "computation");
    
    // Your computation here
    // ...
    
    // Stop measuring
    perf_region_stop(1);
    
    // Output results
    perf_regions_output_human_readable_text();
    
    // Cleanup
    perf_regions_finalize();
    return 0;
}
```

### Using Instrumentation Pragmas
```c
#pragma perf_regions include

int main() {
#pragma perf_regions init
#pragma perf_regions start computation
    
    // Your computation here
    
#pragma perf_regions stop computation
    
    return 0;
}
```

### Fortran API
```fortran
program example
    use perf_regions_fortran
    
    call perf_regions_init()
    call perf_region_start(1, "computation")
    
    ! Your computation here
    
    call perf_region_stop(1)
    call perf_regions_output_human_readable_text()
    call perf_regions_finalize()
end program
```

## PAPI Setup

For hardware performance counters, install PAPI and set environment variables:

```bash
# Check available events
papi_avail

# If events are not available, add to /etc/sysctl.conf:
# kernel.perf_event_paranoid = -1
```

## Using the Library in Your Project

### With CMake
```cmake
find_package(perf_regions REQUIRED)
target_link_libraries(your_target PRIVATE perf_regions::perf_regions)
```

### With pkg-config
```bash
gcc $(pkg-config --cflags --libs perf_regions) your_program.c
```

## Examples

The repository includes several examples in the `examples/` directory:

- **array_test_c**: C array processing example
- **array_test_f**: Fortran array processing example  
- **stream_benchmark**: STREAM benchmark with perf regions
- **nemo_test_f**: NEMO ocean model test case
- **nested_test_f**: Nested regions example

Each example can be built and run:
```bash
# Traditional build
cd examples/array_test_c
make && ./run_tests.sh

# CMake build
cd build && make array_test && ctest -R array_test
```

## Configuration Summary

When building, the system will display a configuration summary:
```
perf_regions 1.0.0 configuration summary:
  Build type: Release
  C compiler: GNU 11.4.0
  Fortran compiler: GNU 11.4.0
  Install prefix: /usr/local
  PAPI support: OFF
  MPI support: ON
  Build examples: ON
  Build shared libs: ON
```

## Directory Structure

```
src/          # Library source code
examples/     # Example programs and tests
cmake/        # CMake configuration files
scripts/      # Instrumentation scripts
doc/          # Documentation
```

## Support

The library provides comprehensive CMake integration with:
- Export targets for easy integration
- Version compatibility checking
- Automatic dependency resolution
- Package configuration files
- CTest integration for testing

For issues and contributions, please use the project repository.
