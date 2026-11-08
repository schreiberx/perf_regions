# Traditional Makefile for perf_regions library
# This provides the traditional build system support

# Include configuration if available
-include ../config.mk

# Compiler settings
CC ?= gcc
FC ?= gfortran

# Compiler flags
CFLAGS ?= -fPIC -Wall -O2
FCFLAGS ?= -fPIC -Wall -O2
CPPFLAGS ?=

# Configuration used to generate the installed configuration header
PERF_DEBUG ?= 0
PERF_COUNTERS_NESTED ?= 1
PERF_TIMING_POSIX ?= 1
USE_FORTRAN ?= 0

# Library build options
BUILD_SHARED_LIBS ?= 1
BUILD_STATIC_LIBS ?= 0

# Shared library flags
SHARED_LDFLAGS = -shared

# MPI settings
USE_MPI ?= 1
ifeq ($(USE_MPI),1)
MPI_CC ?= mpicc
MPI_FC ?= mpif90
CC = $(MPI_CC)
FC = $(MPI_FC)
CFLAGS += -DUSE_MPI=1
else
CFLAGS += -DUSE_MPI=0
endif

# PAPI settings
USE_PAPI ?= 1
ifeq ($(USE_PAPI),1)
CFLAGS += -DPERF_REGIONS_USE_PAPI=1 $(PAPI_CFLAGS)
LDFLAGS += $(PAPI_LDFLAGS)
else
CFLAGS += -DPERF_REGIONS_USE_PAPI=0
endif

# Install settings
PREFIX ?= /usr/local
DESTDIR ?=
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include
BINDIR = $(PREFIX)/bin
BUILD_DIR ?= ../build

INSTALL_LIBDIR = $(DESTDIR)$(LIBDIR)
INSTALL_INCLUDEDIR = $(DESTDIR)$(INCLUDEDIR)
INSTALL_BINDIR = $(DESTDIR)$(BINDIR)

CONFIG_HEADER = $(BUILD_DIR)/perf_regions_config.h

# Object files
C_SOURCES = perf_regions.c perf_regions_output.c posix_clock.c papi_counters.c
F_SOURCES = perf_regions_fortran.F90

C_OBJECTS = $(addprefix $(BUILD_DIR)/, $(C_SOURCES:.c=.o))
ifeq ($(USE_FORTRAN),1)
F_OBJECTS = $(addprefix $(BUILD_DIR)/, $(F_SOURCES:.F90=.o))
else
F_OBJECTS =
endif

# Library names
ifeq ($(USE_MPI),1)
LIB_SUFFIX_MPI = _mpi
else
LIB_SUFFIX_MPI = 
endif

ifeq ($(USE_PAPI),1)
LIB_SUFFIX_PAPI = _papi
else
LIB_SUFFIX_PAPI = 
endif

ifeq ($(USE_FORTRAN),1)
LIB_SUFFIX_FORTRAN = _fort
else
LIB_SUFFIX_FORTRAN = 
endif

LIB_SUFFIX = $(LIB_SUFFIX_PAPI)$(LIB_SUFFIX_MPI)$(LIB_SUFFIX_FORTRAN)

SHARED_LIBRARY = $(BUILD_DIR)/libperf_regions$(LIB_SUFFIX).so
SHARED_LIBRARY_VERSION = $(BUILD_DIR)/libperf_regions$(LIB_SUFFIX).so.1.0.0
SHARED_LIBRARY_SONAME = $(BUILD_DIR)/libperf_regions$(LIB_SUFFIX).so.1

STATIC_LIBRARY = $(BUILD_DIR)/libperf_regions$(LIB_SUFFIX).a

# Headers to install (excluding internal implementation headers)
INSTALL_HEADERS = perf_regions.h perf_regions_defines.h

# Determine what to build
TARGETS = 
ifeq ($(BUILD_SHARED_LIBS),1)
TARGETS += $(SHARED_LIBRARY)
endif
ifeq ($(BUILD_STATIC_LIBS),1)
TARGETS += $(STATIC_LIBRARY)
endif

# Validate that at least one library type is selected
ifeq ($(TARGETS),)
$(error At least one of BUILD_SHARED_LIBS or BUILD_STATIC_LIBS must be set to 1)
endif

# Default target
all: $(CONFIG_HEADER) $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Generate a header that records the options this library was built with.
# The file is only rewritten when its contents change, so it does not force
# unnecessary rebuilds.
$(CONFIG_HEADER): FORCE
	@mkdir -p $(BUILD_DIR)
	@{ \
		echo '#ifndef PERF_REGIONS_CONFIG_H'; \
		echo '#define PERF_REGIONS_CONFIG_H'; \
		echo '#ifndef USE_MPI'; \
		echo '#define USE_MPI $(USE_MPI)'; \
		echo '#endif'; \
		echo '#ifndef PERF_REGIONS_USE_PAPI'; \
		echo '#define PERF_REGIONS_USE_PAPI $(USE_PAPI)'; \
		echo '#endif'; \
		echo '#ifndef USE_FORTRAN'; \
		echo '#define USE_FORTRAN $(USE_FORTRAN)'; \
		echo '#endif'; \
		echo '#ifndef PERF_COUNTERS_NESTED'; \
		echo '#define PERF_COUNTERS_NESTED $(PERF_COUNTERS_NESTED)'; \
		echo '#endif'; \
		echo '#ifndef PERF_TIMING_POSIX'; \
		echo '#define PERF_TIMING_POSIX $(PERF_TIMING_POSIX)'; \
		echo '#endif'; \
		echo '#ifndef PERF_DEBUG'; \
		echo '#define PERF_DEBUG $(PERF_DEBUG)'; \
		echo '#endif'; \
		echo '#endif'; \
	} > $@.tmp
	@if [ -f $@ ] && cmp -s $@ $@.tmp; then rm -f $@.tmp; else mv $@.tmp $@; fi

.PHONY: FORCE
FORCE:

# Build the shared library
$(SHARED_LIBRARY): $(C_OBJECTS) $(F_OBJECTS)
ifeq ($(USE_FORTRAN),1)
	$(FC) $(SHARED_LDFLAGS) -Wl,-soname,$(notdir $(SHARED_LIBRARY_SONAME)) -Wl,-rpath,$(LIBDIR) -o $(SHARED_LIBRARY_VERSION) $^ $(LDFLAGS)
else
	$(CC) $(SHARED_LDFLAGS) -Wl,-soname,$(notdir $(SHARED_LIBRARY_SONAME)) -Wl,-rpath,$(LIBDIR) -o $(SHARED_LIBRARY_VERSION) $^ $(LDFLAGS)
endif
	ln -sf $(notdir $(SHARED_LIBRARY_VERSION)) $(SHARED_LIBRARY_SONAME)
	ln -sf $(notdir $(SHARED_LIBRARY_SONAME)) $(SHARED_LIBRARY)

# Build the static library
$(STATIC_LIBRARY): $(C_OBJECTS) $(F_OBJECTS)
	ar rcs $@ $^

$(C_OBJECTS) $(F_OBJECTS): $(CONFIG_HEADER)

# C object files
$(BUILD_DIR)/%.o: %.c
	$(CC) $(CPPFLAGS) -I$(BUILD_DIR) $(CFLAGS) -MMD -MP -c $< -o $@

# Fortran object files
$(BUILD_DIR)/%.o: %.F90
ifeq ($(USE_FORTRAN),1)
	$(FC) -I$(BUILD_DIR) $(FCFLAGS) -c $< -o $@
endif

# Install target
install: $(TARGETS) $(CONFIG_HEADER)
	@echo "Installing perf_regions to $(DESTDIR)$(PREFIX)"
	install -d $(INSTALL_LIBDIR)
	install -d $(INSTALL_INCLUDEDIR)
	install -d $(INSTALL_BINDIR)
ifeq ($(BUILD_SHARED_LIBS),1)
	install -m 755 $(SHARED_LIBRARY_VERSION) $(INSTALL_LIBDIR)/
	cd $(INSTALL_LIBDIR) && ln -sf $(notdir $(SHARED_LIBRARY_VERSION)) $(notdir $(SHARED_LIBRARY_SONAME))
	cd $(INSTALL_LIBDIR) && ln -sf $(notdir $(SHARED_LIBRARY_SONAME)) $(notdir $(SHARED_LIBRARY))
endif
ifeq ($(BUILD_STATIC_LIBS),1)
	install -m 644 $(STATIC_LIBRARY) $(INSTALL_LIBDIR)/
endif
	install -m 644 $(INSTALL_HEADERS) $(INSTALL_INCLUDEDIR)/
	install -m 644 $(CONFIG_HEADER) $(INSTALL_INCLUDEDIR)/perf_regions_config.h
	install -m 644 $(BUILD_DIR)/*.mod $(INSTALL_INCLUDEDIR)/ 2>/dev/null || true
	install -m 644 *.mod $(INSTALL_INCLUDEDIR)/ 2>/dev/null || true
	install -m 755 ../scripts/perf_regions.py $(INSTALL_BINDIR)/

# Uninstall target
uninstall:
	@echo "Uninstalling perf_regions from $(DESTDIR)$(PREFIX)"
	rm -f $(INSTALL_LIBDIR)/$(notdir $(SHARED_LIBRARY_VERSION))
	rm -f $(INSTALL_LIBDIR)/$(notdir $(SHARED_LIBRARY_SONAME))
	rm -f $(INSTALL_LIBDIR)/$(notdir $(SHARED_LIBRARY))
	rm -f $(INSTALL_LIBDIR)/$(notdir $(STATIC_LIBRARY))
	rm -f $(INSTALL_INCLUDEDIR)/perf_regions.h
	rm -f $(INSTALL_INCLUDEDIR)/perf_regions_defines.h
	rm -f $(INSTALL_INCLUDEDIR)/perf_regions_config.h
	rm -f $(INSTALL_INCLUDEDIR)/perf_regions_fortran.mod
	rm -f $(INSTALL_BINDIR)/perf_regions.py

# Clean target
clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d $(BUILD_DIR)/*.mod
	rm -f $(BUILD_DIR)/$(notdir $(SHARED_LIBRARY)) $(BUILD_DIR)/$(notdir $(SHARED_LIBRARY_VERSION))
	rm -f $(BUILD_DIR)/$(notdir $(SHARED_LIBRARY_SONAME)) $(BUILD_DIR)/$(notdir $(STATIC_LIBRARY))
	rm -f $(CONFIG_HEADER)
	rm -rf $(BUILD_DIR)
# Some compilers have leftover files here
	rm -f *.mod

.PHONY: all install uninstall clean FORCE

# Header dependencies generated by -MMD
-include $(C_OBJECTS:.o=.d)
