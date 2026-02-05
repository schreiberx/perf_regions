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
CFLAGS += -DPERF_REGIONS_USE_PAPI=1 -I$(PREFIX)/include
# Use PAPI from installation prefix - explicit path if LIBDIR is empty
ifeq ($(strip $(LIBDIR)),)
PAPI_LIBS := -L$(PREFIX)/lib -lpapi -lpfm
else
PAPI_LIBS := -L$(LIBDIR) -lpapi -lpfm
endif
LDFLAGS += $(PAPI_LIBS)
else
CFLAGS += -DPERF_REGIONS_USE_PAPI=0
PAPI_LIBS :=
endif

# Install settings
PREFIX ?= /usr/local
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include
BINDIR = $(PREFIX)/bin

# Object files
C_SOURCES = perf_regions.c perf_regions_output.c posix_clock.c papi_counters.c
F_SOURCES = perf_regions_fortran.F90

C_OBJECTS = $(C_SOURCES:.c=.o)
F_OBJECTS = $(F_SOURCES:.F90=.o)

# Library names
SHARED_LIBRARY = libperf_regions.so
SHARED_LIBRARY_VERSION = libperf_regions.so.1.0.0
SHARED_LIBRARY_SONAME = libperf_regions.so.1

STATIC_LIBRARY = libperf_regions.a

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
all: $(TARGETS)

# Build the shared library
$(SHARED_LIBRARY): $(C_OBJECTS) $(F_OBJECTS)
	$(FC) $(SHARED_LDFLAGS) -Wl,-soname,$(SHARED_LIBRARY_SONAME) -Wl,-rpath,$(LIBDIR) -o $(SHARED_LIBRARY_VERSION) $^ $(PAPI_LIBS)
	ln -sf $(SHARED_LIBRARY_VERSION) $(SHARED_LIBRARY_SONAME)
	ln -sf $(SHARED_LIBRARY_SONAME) $(SHARED_LIBRARY)

# Build the static library
$(STATIC_LIBRARY): $(C_OBJECTS) $(F_OBJECTS)
	ar rcs $@ $^

# C object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Fortran object files  
%.o: %.F90
	$(FC) $(FCFLAGS) -c $< -o $@

# Install target
install: $(TARGETS)
	@echo "Installing perf_regions to $(PREFIX)"
	install -d $(LIBDIR)
	install -d $(INCLUDEDIR)
	install -d $(BINDIR)
ifeq ($(BUILD_SHARED_LIBS),1)
	install -m 755 $(SHARED_LIBRARY_VERSION) $(LIBDIR)/
	cd $(LIBDIR) && ln -sf $(SHARED_LIBRARY_VERSION) $(SHARED_LIBRARY_SONAME)
	cd $(LIBDIR) && ln -sf $(SHARED_LIBRARY_SONAME) $(SHARED_LIBRARY)
endif
ifeq ($(BUILD_STATIC_LIBS),1)
	install -m 644 $(STATIC_LIBRARY) $(LIBDIR)/
endif
	install -m 644 $(INSTALL_HEADERS) $(INCLUDEDIR)/
	install -m 644 *.mod $(INCLUDEDIR)/ 2>/dev/null || true
	install -m 755 ../scripts/perf_regions.py $(BINDIR)/

# Uninstall target
uninstall:
	@echo "Uninstalling perf_regions from $(PREFIX)"
	rm -f $(LIBDIR)/$(SHARED_LIBRARY_VERSION)
	rm -f $(LIBDIR)/$(SHARED_LIBRARY_SONAME) 
	rm -f $(LIBDIR)/$(SHARED_LIBRARY)
	rm -f $(LIBDIR)/$(STATIC_LIBRARY)
	rm -rf $(INCLUDEDIR)
	rm -f $(BINDIR)/perf_regions.py

# Clean target
clean:
	rm -f *.o *.mod $(SHARED_LIBRARY) $(SHARED_LIBRARY_VERSION) $(SHARED_LIBRARY_SONAME) $(STATIC_LIBRARY)

.PHONY: all install uninstall clean
