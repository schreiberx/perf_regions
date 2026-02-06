
# Traditional Makefile build only (CMake support removed)

# Include configuration if available
-include config.mk

# Default values
PREFIX ?= /usr/local
USE_MPI ?= 1
USE_PAPI ?= 1
BUILD_SHARED_LIBS ?= 1
BUILD_STATIC_LIBS ?= 0

.PHONY: all build_src tests run_tests clean help install uninstall distclean

# Default target
all: build_src

build_src:
	$(MAKE) -C ./src USE_MPI=$(USE_MPI) USE_PAPI=$(USE_PAPI) BUILD_SHARED_LIBS=$(BUILD_SHARED_LIBS) BUILD_STATIC_LIBS=$(BUILD_STATIC_LIBS)

install: build_src
	$(MAKE) -C ./src install PREFIX=$(PREFIX) USE_MPI=$(USE_MPI) USE_PAPI=$(USE_PAPI) BUILD_SHARED_LIBS=$(BUILD_SHARED_LIBS) BUILD_STATIC_LIBS=$(BUILD_STATIC_LIBS)

uninstall:
	$(MAKE) -C ./src uninstall PREFIX=$(PREFIX)

clean:
	$(MAKE) -C ./src clean
	$(MAKE) -C ./tests clean

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
	@echo "  tests                  - Build tests"
	@echo "  run_tests                 - Run tests"
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
	@echo "  BUILD_SHARED_LIBS         - Build shared libraries (0/1, default: 1)"
	@echo "  BUILD_STATIC_LIBS         - Build static libraries (0/1, default: 0)"
	@echo "  PREFIX                    - Install prefix (default: /usr/local)"
	@echo "  DESTDIR                   - Destination directory for staged installs"

# Clean all generated files including configure outputs  
distclean:
	$(MAKE) clean 2>/dev/null || true
	rm -rf build-* *.log
	@echo "Complete cleanup finished"
