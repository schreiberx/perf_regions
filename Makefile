
# Default build method - can be overridden with BUILD_METHOD=cmake
BUILD_METHOD ?= make
BUILD_DIR ?= build
PREFIX ?= /usr/local

# CMake options
CMAKE_BUILD_TYPE ?= Release
CMAKE_OPTIONS ?= -DPERF_REGIONS_USE_MPI=ON -DPERF_REGIONS_BUILD_EXAMPLES=ON

.PHONY: all build_src examples run_tests clean help cmake-configure cmake-build cmake-clean distclean

# Check if configure was used (build directory with Makefile exists)
ifneq (,$(wildcard build/Makefile))
# Configure-based build

all examples: 
	@echo "Using configured build (run by ./configure)"
	@cd build && $(MAKE) $@

build_src: all

# Pass through any other targets to the configured build system
%:
	@cd build && $(MAKE) $@

install:
	@cd build && $(MAKE) install

run_tests:
	@cd build && ctest --output-on-failure

clean:
	@cd build && $(MAKE) clean

else
# Direct build method selection

# Default target (suggest using configure)
all:
	@echo "============================================"
	@echo "Welcome to perf_regions build system!"
	@echo "============================================"
	@echo ""
	@echo "Recommended: Use the configure script"
	@echo "  ./configure && make"
	@echo ""
	@echo "Alternative: Use specific build method"
	@echo "  make BUILD_METHOD=cmake"
	@echo "  make BUILD_METHOD=make USE_MPI=0"
	@echo ""
	@echo "For help:"
	@echo "  make help"
	@echo "  ./configure --help"

# Default target
build_src: all

# Traditional Makefile build
ifeq ($(BUILD_METHOD),make)

build_src:
	make -C ./src MODE=${MODE} USE_MPI=${USE_MPI} USE_PAPI=${USE_PAPI}

examples:
	make -C ./examples MODE=${MODE} USE_MPI=${USE_MPI} USE_PAPI=${USE_PAPI}

run_tests:
	make -C ./examples run_tests MODE=${MODE} USE_MPI=${USE_MPI} USE_PAPI=${USE_PAPI}

install: build_src
	make -C ./src install PREFIX=${PREFIX} USE_MPI=${USE_MPI} USE_PAPI=${USE_PAPI}

uninstall:
	make -C ./src uninstall PREFIX=${PREFIX} USE_MPI=${USE_MPI} USE_PAPI=${USE_PAPI}

clean:
	make -C ./src clean MODE=${MODE} USE_MPI=${USE_MPI} USE_PAPI=${USE_PAPI}
	make -C ./examples clean MODE=${MODE} USE_MPI=${USE_MPI} USE_PAPI=${USE_PAPI}

# CMake build
else ifeq ($(BUILD_METHOD),cmake)

build_src: cmake-build

examples: cmake-build

cmake-configure:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_OPTIONS) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) ..

cmake-build: cmake-configure
	cd $(BUILD_DIR) && $(MAKE)

run_tests: cmake-build
	cd $(BUILD_DIR) && ctest --output-on-failure

clean: cmake-clean

cmake-clean:
	rm -rf $(BUILD_DIR)

endif

endif

# Convenience targets for CMake
cmake: cmake-build

package: build_src
ifeq ($(BUILD_METHOD),cmake)
	cd $(BUILD_DIR) && $(MAKE) package
else
	@echo "Package target only available with CMake build (BUILD_METHOD=cmake)"
endif

help:
	@echo "perf_regions build system"
	@echo ""
	@echo "Build methods:"
	@echo "  ./configure && make       - Autotools-style configuration and build (recommended)"
	@echo "  make BUILD_METHOD=make    - Traditional Makefile build"
	@echo "  make BUILD_METHOD=cmake   - Modern CMake build"
	@echo ""
	@echo "Targets:"
	@echo "  all                       - Build the library"
	@echo "  build_src                 - Build the library"
	@echo "  examples                  - Build examples"
	@echo "  run_tests                 - Run tests"
	@echo "  install                   - Install library and headers"
	@echo "  uninstall                 - Uninstall library and headers (traditional build only)"
	@echo "  clean                     - Clean build artifacts"
	@echo "  distclean                 - Clean all generated files including configure outputs"
	@echo "  package                   - Create packages (CMake only)"
	@echo ""
	@echo "CMake-specific targets:"
	@echo "  cmake-configure           - Configure CMake build"
	@echo "  cmake-build              - Build with CMake"
	@echo "  cmake-clean              - Clean CMake build"
	@echo ""
	@echo "Configuration:"
	@echo "  ./configure --help        - Show configuration options"
	@echo ""
	@echo "Environment variables:"
	@echo "  MODE                      - Build mode for traditional Makefiles (debug/release)"
	@echo "  PREFIX                    - Install prefix (default: /usr/local)"
	@echo "  DESTDIR                   - Destination directory for staged installs"
	@echo "  BUILD_DIR                 - CMake build directory (default: build)"
	@echo "  CMAKE_BUILD_TYPE          - CMake build type (default: Release)"
	@echo "  CMAKE_OPTIONS             - Additional CMake options"

# Clean all generated files including configure outputs  
distclean:
	make clean BUILD_METHOD=make 2>/dev/null || true
	make cmake-clean 2>/dev/null || true
	rm -rf build build-* *.log
	rm -f configure_done.txt
	@echo "Complete cleanup finished"
