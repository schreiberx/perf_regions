
# Traditional Makefile build only

# Include configuration if available
-include config.mk

# Default values
PREFIX ?= /usr/local
DESTDIR ?=
USE_MPI ?= 1
USE_PAPI ?= 1
USE_FORTRAN ?= 0
BUILD_SHARED_LIBS ?= 1
BUILD_STATIC_LIBS ?= 0
PYTHON ?= python3
PAPI_MODE ?= local
PAPI_PREFIX ?=
PAPI_COUNTERS ?= PAPI_TOT_CYC,WALLCLOCKTIME

.PHONY: all build_src tests test test_papi test_hardware test_all test-all test_papi_through_perf_regions run_tests clean help install uninstall distclean

# Default target
all: build_src

build_src:
	$(MAKE) -C ./src USE_MPI=$(USE_MPI) USE_PAPI=$(USE_PAPI) USE_FORTRAN=$(USE_FORTRAN) BUILD_SHARED_LIBS=$(BUILD_SHARED_LIBS) BUILD_STATIC_LIBS=$(BUILD_STATIC_LIBS)

tests: test

test:
	$(PYTHON) tests/run_tests.py --fast

test_papi:
	$(PYTHON) tests/run_tests.py --papi local --test-papi-counters --fast

test_hardware:
	$(PYTHON) tests/run_tests.py --papi local --test-papi-counters --pytest-args '-m papi_hardware'

test_all test-all:
	./tests/run_tests_all.sh

test_papi_through_perf_regions:
	$(PYTHON) tests/run_tests.py \
		--papi "$(PAPI_MODE)" \
		$(if $(PAPI_PREFIX),--papi-prefix "$(PAPI_PREFIX)") \
		--test-papi-counters \
		--require-papi-hardware \
		--counters "$(PAPI_COUNTERS)" \
		--pytest-target hardware/papi/perf_regions_counters \
		--pytest-args '-k test_papi_counter_through_perf_regions'

run_tests:
	$(MAKE) -C ./tests run_tests

install: build_src
	$(MAKE) -C ./src install PREFIX=$(PREFIX) DESTDIR=$(DESTDIR) USE_MPI=$(USE_MPI) USE_PAPI=$(USE_PAPI) USE_FORTRAN=$(USE_FORTRAN) BUILD_SHARED_LIBS=$(BUILD_SHARED_LIBS) BUILD_STATIC_LIBS=$(BUILD_STATIC_LIBS)

uninstall:
	$(MAKE) -C ./src uninstall PREFIX=$(PREFIX) DESTDIR=$(DESTDIR) USE_MPI=$(USE_MPI) USE_PAPI=$(USE_PAPI) USE_FORTRAN=$(USE_FORTRAN) BUILD_SHARED_LIBS=$(BUILD_SHARED_LIBS) BUILD_STATIC_LIBS=$(BUILD_STATIC_LIBS)

clean:
	$(MAKE) -C ./src clean
	$(MAKE) -C ./tests clean
	rm -rf build build_*
	rm -f *.mod

help:
	@echo "perf_regions build system (traditional make only)"
	@echo ""
	@echo "Build methods:"
	@echo "  ./configure && make       - Autotools-style configuration and build (recommended)"
	@echo "  make                      - Direct traditional Makefile build"
	@echo ""
	@echo "Targets:"
	@echo "  all                       - Build the library"
	@echo "  build_src                 - Build the library"
	@echo "  test / tests              - Run the fast default test suite"
	@echo "  test_papi                 - Run PAPI-enabled core tests"
	@echo "  test_hardware             - Run PAPI hardware tests"
	@echo "  test_all / test-all       - Run the full compile/runtime matrix"
	@echo "  test_papi_through_perf_regions - Check PAPI access through PerfRegions"
	@echo "  install                   - Install library and headers"
	@echo "  uninstall                 - Uninstall library and headers"
	@echo "  clean                     - Clean build artifacts"
	@echo "  distclean                 - Clean all generated files including configure outputs"
	@echo ""
	@echo "Configuration:"
	@echo "  ./configure --help        - Show configuration options"
	@echo ""
	@echo "Environment variables:"
	@echo "  USE_MPI                   - Enable MPI support (0/1, default: 1)"
	@echo "  USE_PAPI                  - Enable PAPI support (0/1, default: 1)"
	@echo "  USE_FORTRAN               - Build the Fortran interface (0/1, default: 0)"
	@echo "  BUILD_SHARED_LIBS         - Build shared libraries (0/1, default: 1)"
	@echo "  BUILD_STATIC_LIBS         - Build static libraries (0/1, default: 0)"
	@echo "  PREFIX                    - Install prefix (default: /usr/local)"
	@echo "  DESTDIR                   - Destination directory for staged installs"

# Clean all generated files including configure outputs  
distclean:
	$(MAKE) clean 2>/dev/null || true
	rm -rf build-* *.log
	rm -f src/Makefile.deps build.sh install.sh
	@echo "Complete cleanup finished"
