
#CC = icc
CC = gcc
CFLAGS = -O2 -fopenmp

#FF = ifort
FF = gfortran
FFLAGS = -O2 -fopenmp

# Use traditional Makefile build (CMake support removed)
PERF_REGIONS_DIR=../../
PERF_REGIONS_LIB_DIR=$(PERF_REGIONS_DIR)/src

#all: stream_f.exe stream_c.exe stream_c_perfregion.exe
all: build

build:
	make -C ../../src
	make stream_c_perfregion.exe

run_tests: build
	./run_tests.sh

stream_f.exe: stream.f mysecond.o
	$(CC) $(CFLAGS) -c mysecond.c
	$(FF) $(FFLAGS) -c stream.f
	$(FF) $(FFLAGS) stream.o mysecond.o -o stream_f.exe

stream_c.exe: stream.c
	./perf_regions_instrumentation.py cleanup
	$(CC) $(CFLAGS) stream.c -o stream_c.exe

# PAPI settings
USE_PAPI ?= 1
ifeq ($(USE_PAPI),1)
PAPI_LINK_FLAG := -lpapi
else
PAPI_LINK_FLAG :=
endif

stream_c_perfregion.exe: stream.c
	./perf_regions_instrumentation.py
	$(CC) $(CFLAGS) stream.c -o stream_c_perfregions.exe -I$(PERF_REGIONS_DIR)/src -L$(PERF_REGIONS_LIB_DIR) $(PAPI_LINK_FLAG) -lperf_regions
	./perf_regions_instrumentation.py cleanup

# an example of a more complex build line for the Intel icc compiler
stream.icc: stream.c
	icc -O3 -xCORE-AVX2 -ffreestanding -qopenmp -DSTREAM_ARRAY_SIZE=80000000 -DNTIMES=20 stream.c -o stream.omp.AVX2.80M.20x.icc

clean:
	./perf_regions_instrumentation.py cleanup
	rm -f stream_f.exe stream_c.exe stream_c_perfregions.exe stream_test stream_test_perf_region *.o *.mod
	rm -f perf_region_list.txt

.PHONY: all build run_tests stream_c_perfregion.exe clean
