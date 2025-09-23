# Traditional Makefile for perf_regions library
# This provides the traditional build system support

# Compiler settings
CC ?= gcc
FC ?= gfortran

# Compiler flags
CFLAGS ?= -fPIC -Wall -O2
FCFLAGS ?= -fPIC -Wall -O2
LDFLAGS ?= -shared

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
CFLAGS += -DPERF_REGIONS_USE_PAPI=1
LDFLAGS += -lpapi
else
CFLAGS += -DPERF_REGIONS_USE_PAPI=0
endif

# Install settings
PREFIX ?= /usr/local
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include/perf_regions
BINDIR = $(PREFIX)/bin

# Object files
C_SOURCES = perf_regions.c posix_clock.c papi_counters.c
F_SOURCES = perf_regions_fortran.F90

C_OBJECTS = $(C_SOURCES:.c=.o)
F_OBJECTS = $(F_SOURCES:.F90=.o)

LIBRARY = libperf_regions.so
LIBRARY_VERSION = libperf_regions.so.1.0.0
LIBRARY_SONAME = libperf_regions.so.1

# Headers to install (excluding internal implementation headers)
INSTALL_HEADERS = perf_regions.h perf_regions_defines.h

# Default target
all: $(LIBRARY)

# Build the shared library
$(LIBRARY): $(C_OBJECTS) $(F_OBJECTS)
	$(FC) $(LDFLAGS) -Wl,-soname,$(LIBRARY_SONAME) -o $(LIBRARY_VERSION) $^ $(LDFLAGS)
	ln -sf $(LIBRARY_VERSION) $(LIBRARY_SONAME)
	ln -sf $(LIBRARY_SONAME) $(LIBRARY)

# C object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Fortran object files  
%.o: %.F90
	$(FC) $(FCFLAGS) -c $< -o $@

# Install target
install: $(LIBRARY)
	@echo "Installing perf_regions to $(PREFIX)"
	install -d $(LIBDIR)
	install -d $(INCLUDEDIR)
	install -d $(BINDIR)
	install -m 755 $(LIBRARY_VERSION) $(LIBDIR)/
	cd $(LIBDIR) && ln -sf $(LIBRARY_VERSION) $(LIBRARY_SONAME)
	cd $(LIBDIR) && ln -sf $(LIBRARY_SONAME) $(LIBRARY)
	install -m 644 $(INSTALL_HEADERS) $(INCLUDEDIR)/
	install -m 644 *.mod $(INCLUDEDIR)/ 2>/dev/null || true
	install -m 755 ../scripts/perf_regions.py $(BINDIR)/

# Uninstall target
uninstall:
	@echo "Uninstalling perf_regions from $(PREFIX)"
	rm -f $(LIBDIR)/$(LIBRARY_VERSION)
	rm -f $(LIBDIR)/$(LIBRARY_SONAME) 
	rm -f $(LIBDIR)/$(LIBRARY)
	rm -rf $(INCLUDEDIR)
	rm -f $(BINDIR)/perf_regions.py

# Clean target
clean:
	rm -f *.o *.mod $(LIBRARY) $(LIBRARY_VERSION) $(LIBRARY_SONAME)

.PHONY: all install uninstall clean
