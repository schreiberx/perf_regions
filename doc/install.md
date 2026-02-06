
# Installing PerfRegions

## Configuration

We can configure the build process using the `configure` program. 
E.g., one can use

```
$ ./configure --enable-debug --prefix=/usr/local
```
to configure the build. This will create the file `config.mk` containing all default settings for the build process.

Use
```
$ ./configure --help
```
to see all kinds of compilation options.

## Build & install

Build and install PerfRegions by executing

```
$ ./build.sh && ./install.sh
```


# Information about 3rd party library, compiling and linking

PerfRegions requires the PAPI library installed.

## Linker flags:

For non-MPI builds:
```
-lpapi -L[path to perf regions]/build -lperf_regions 
```

For MPI builds:
```
-lpapi -L[path to perf regions]/build -lperf_regions_mpi 
```

## Compile flags:

```
-I[path to perf regions]/src 
```
