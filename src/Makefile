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
CFLAGS += -DPERF_REGIONS_USE_PAPI=1 $(PAPI_CFLAGS)
LDFLAGS += $(PAPI_LDFLAGS)
else
CFLAGS += -DPERF_REGIONS_USE_PAPI=0
endif

# Install settings
PREFIX ?= /usr/local
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include
BINDIR = $(PREFIX)/bin
BUILD_DIR ?= ../build

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
all: $(BUILD_DIR) $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

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

# C object files
$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Fortran object files  
$(BUILD_DIR)/%.o: %.F90
ifeq ($(USE_FORTRAN),1)
	$(FC) $(FCFLAGS) -c $< -o $@
endif

# Install target
install: $(TARGETS)
	@echo "Installing perf_regions to $(PREFIX)"
	install -d $(LIBDIR)
	install -d $(INCLUDEDIR)
	install -d $(BINDIR)
ifeq ($(BUILD_SHARED_LIBS),1)
	install -m 755 $(SHARED_LIBRARY_VERSION) $(LIBDIR)/
	cd $(LIBDIR) && ln -sf $(notdir $(SHARED_LIBRARY_VERSION)) $(notdir $(SHARED_LIBRARY_SONAME))
	cd $(LIBDIR) && ln -sf $(notdir $(SHARED_LIBRARY_SONAME)) $(notdir $(SHARED_LIBRARY))
endif
ifeq ($(BUILD_STATIC_LIBS),1)
	install -m 644 $(STATIC_LIBRARY) $(LIBDIR)/
endif
	install -m 644 $(INSTALL_HEADERS) $(INCLUDEDIR)/
	install -m 644 $(BUILD_DIR)/*.mod $(INCLUDEDIR)/ 2>/dev/null || true
	install -m 644 *.mod $(INCLUDEDIR)/ 2>/dev/null || true
	install -m 755 ../scripts/perf_regions.py $(BINDIR)/

# Uninstall target
uninstall:
	@echo "Uninstalling perf_regions from $(PREFIX)"
	rm -f $(LIBDIR)/$(notdir $(SHARED_LIBRARY_VERSION))
	rm -f $(LIBDIR)/$(notdir $(SHARED_LIBRARY_SONAME)) 
	rm -f $(LIBDIR)/$(notdir $(SHARED_LIBRARY))
	rm -f $(LIBDIR)/$(notdir $(STATIC_LIBRARY))
	rm -rf $(INCLUDEDIR)
	rm -f $(BINDIR)/perf_regions.py

# Clean target
clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.mod $(SHARED_LIBRARY) $(SHARED_LIBRARY_VERSION) $(SHARED_LIBRARY_SONAME) $(STATIC_LIBRARY)
	rm -rf $(BUILD_DIR)
# Some compilers have leftover files here
	rm -f *.mod

.PHONY: all install uninstall clean
