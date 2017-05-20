

#CC = icc
CC = gcc
CFLAGS = -O2

#FF = ifort
FF = gfortran
FFLAGS = -O2

PERF_REGIONS_DIR=../../

all: stream_f.exe stream_c.exe stream_c_perfregion.exe

stream_f.exe: stream.f mysecond.o
	$(CC) $(CFLAGS) -c mysecond.c
	$(FF) $(FFLAGS) -c stream.f
	$(FF) $(FFLAGS) stream.o mysecond.o -o stream_f.exe

stream_c.exe: stream.c
	./perf_regions_instrumentation.py cleanup
	$(CC) $(CFLAGS) stream.c -o stream_c.exe

stream_c_perfregion.exe: stream.c
	./perf_regions_instrumentation.py
	$(CC) $(CFLAGS) stream.c -o stream_c_perfregions.exe -I$(PERF_REGIONS_DIR)/src -L$(PERF_REGIONS_DIR)/build -lpapi -lperf_regions
	./perf_regions_instrumentation.py cleanup

# an example of a more complex build line for the Intel icc compiler
stream.icc: stream.c
	icc -O3 -xCORE-AVX2 -ffreestanding -qopenmp -DSTREAM_ARRAY_SIZE=80000000 -DNTIMES=20 stream.c -o stream.omp.AVX2.80M.20x.icc

clean:
	rm -f stream_f.exe stream_c.exe stream_c_perfregions.exe perf_region_list.txt *.o
	rm -f perf_region_list.txt
