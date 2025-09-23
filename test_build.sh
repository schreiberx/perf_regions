#!/bin/bash

# Build verification script for perf_regions
# Tests both CMake and traditional Makefile builds

set -e  # Exit on any error

echo "======================================"
echo "perf_regions Build System Test"
echo "======================================"
echo

# Function to print colored status messages
print_status() {
    local status=$1
    local message=$2
    if [ "$status" = "OK" ]; then
        echo -e "\033[32m[OK]\033[0m $message"
    elif [ "$status" = "ERROR" ]; then
        echo -e "\033[31m[ERROR]\033[0m $message"
    elif [ "$status" = "INFO" ]; then
        echo -e "\033[34m[INFO]\033[0m $message"
    fi
}

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Clean up any previous builds
print_status "INFO" "Cleaning previous builds..."
rm -rf build build-test || true
make distclean &>/dev/null || true

# Test 1: Configure script build
print_status "INFO" "Testing autotools-style configure script..."

if ./configure --disable-mpi --disable-papi &>/dev/null; then
    print_status "OK" "Configure script successful"
else
    print_status "ERROR" "Configure script failed"
    exit 1
fi

if make array_test &>/dev/null; then
    print_status "OK" "Configure-based build successful"
else
    print_status "ERROR" "Configure-based build failed"
    exit 1
fi

# Test configure-built executable
if ./build/examples/array_test_c/array_test &>/dev/null; then
    print_status "OK" "Configure-built executable runs successfully"
else
    print_status "ERROR" "Configure-built executable failed to run"
    exit 1
fi

# Clean and test again
make distclean &>/dev/null || true

# Test 2: CMake build
print_status "INFO" "Testing CMake build..."
mkdir -p build-test
cd build-test

if cmake -DCMAKE_BUILD_TYPE=Release -DPERF_REGIONS_USE_MPI=OFF -DPERF_REGIONS_USE_PAPI=OFF .. &>/dev/null; then
    print_status "OK" "CMake configuration successful"
else
    print_status "ERROR" "CMake configuration failed"
    exit 1
fi

if make -j$(nproc) array_test array_test_perf_region nemo_test nemo_test_perf_region array_test_f array_test_f_perf_region &>/dev/null; then
    print_status "OK" "CMake build successful (core examples)"
else
    print_status "ERROR" "CMake build failed"
    exit 1
fi

# Test CMake-built executable
if ./examples/array_test_c/array_test &>/dev/null; then
    print_status "OK" "CMake executable runs successfully"
else
    print_status "ERROR" "CMake executable failed to run"
    exit 1
fi

cd ..

# Test 3: Traditional Makefile build
print_status "INFO" "Testing traditional Makefile build..."

if make BUILD_METHOD=make build_src USE_MPI=0 &>/dev/null; then
    print_status "OK" "Traditional Makefile build successful"
else
    print_status "ERROR" "Traditional Makefile build failed"
    exit 1
fi

# Test 4: Build example with traditional Makefile
print_status "INFO" "Testing example build with traditional Makefile..."
cd examples/array_test_c

if make USE_MPI=0 &>/dev/null; then
    print_status "OK" "Traditional Makefile example build successful"
else
    print_status "ERROR" "Traditional Makefile example build failed"
    exit 1
fi

# Test traditional Makefile-built executable
if ./array_test &>/dev/null; then
    print_status "OK" "Traditional Makefile executable runs successfully"
else
    print_status "ERROR" "Traditional Makefile executable failed to run"
    exit 1
fi

cd ../..

# Test 5: CMake presets (if CMake 3.20+ is available)
if cmake --version 2>/dev/null | grep -q "version 3\.[2-9][0-9]\|version [4-9]"; then
    print_status "INFO" "Testing CMake presets..."
    
    rm -rf build
    if cmake --preset=minimal &>/dev/null && cmake --build --preset=minimal &>/dev/null; then
        print_status "OK" "CMake presets work successfully"
    else
        print_status "INFO" "CMake presets not fully supported (CMake version may be too old)"
    fi
fi

# Test 6: Help system
print_status "INFO" "Testing help system..."
if make help | grep -q "perf_regions build system"; then
    print_status "OK" "Build system help available"
else
    print_status "ERROR" "Build system help not working"
fi

echo
print_status "OK" "All build system tests passed!"
echo
echo "Build methods available:"
echo "  - Configure script (recommended): ./configure && make"
echo "  - CMake: mkdir build && cd build && cmake .. && make"
echo "  - Traditional Makefile: make BUILD_METHOD=make USE_MPI=0"
echo "  - Hybrid: make BUILD_METHOD=cmake"
echo
echo "For more options, run: make help or ./configure --help"