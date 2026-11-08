# PerfRegions

PerfRegions is a library and source-instrumentation tool for measuring selected
regions of C and Fortran applications. It combines wallclock timing with
optional PAPI hardware performance counters and supports MPI reductions over
the measured results.

## Features

- Region-based wallclock timing.
- Optional PAPI hardware performance counters.
- Optional MPI support with accumulation over communicator ranks.
- C and Fortran APIs.
- Native `perf_regions` annotations and NEMO `timing_*` compatibility.
- Human-readable console output and machine-readable JSON output.
- Runtime selection of counters, skipped executions, verbosity, and output
  destination through environment variables.
- Automatic source restoration through the instrumentation cleanup mode.

## Documentation

The documentation is available at
[https://schreiberx.github.io/perf_regions/](https://schreiberx.github.io/perf_regions/).

The Sphinx sources live in [`docs/`](docs/). To install the documentation
dependencies and build the HTML locally, run `docs/install.sh`. The generated
HTML is written to `docs/build/html`.

The GitHub Actions workflow
[`.github/workflows/static.yml`](.github/workflows/static.yml) builds the site
for pull requests and publishes it to GitHub Pages on the default branch.

## Dependencies

Required:

- A C compiler such as GCC or Clang.
- `make` and `ar`.
- Python 3 for the instrumentation script.

Optional:

- A Fortran compiler such as `gfortran`, `ifort`, or `nvfortran`.
- MPI compilers such as `mpicc` and `mpif90`.
- PAPI for hardware performance counters.

PerfRegions can be built without MPI, without PAPI, and without Fortran
support.

## Build and install

PerfRegions uses an autotools-style configuration script:

```bash
./configure --enable-mpi --enable-papi
make -j
sudo make install
```

For a minimal build without MPI and PAPI:

```bash
./configure --disable-mpi --disable-papi --enable-fortran
make -j
```

See `./configure --help` for all options and the
[installation guide](docs/source/user_guide/installation.rst) for link flags,
library naming, and runtime paths.

## Instrumenting applications

Annotate regions in C:

```c
#pragma perf_regions include

int main(void)
{
#pragma perf_regions init

#pragma perf_regions start compute
    do_work();
#pragma perf_regions stop compute

#pragma perf_regions finalize
    return 0;
}
```

Annotate regions in Fortran:

```fortran
PROGRAM main
!$perf_regions include

!$perf_regions init

!$perf_regions start compute
    CALL do_work()
!$perf_regions stop compute

!$perf_regions finalize
END PROGRAM main
```

Then run the instrumentation script over all source files in one invocation:

```bash
python3 scripts/perf_regions.py preprocess 'src/*.F90'
```

Use `cleanup` to restore the original source:

```bash
python3 scripts/perf_regions.py cleanup 'src/*.F90'
```

See the [user guide](docs/source/user_guide/index.rst) for the annotation
reference, instrumentation options, runtime environment variables, output
formats, and MPI support.

## Running

PerfRegions reads its configuration at initialization time. For example:

```bash
export PERF_REGIONS_COUNTERS=PAPI_TOT_CYC,PAPI_L3_TCM,WALLCLOCKTIME
export PERF_REGIONS_SKIP_N=2
export PERF_REGIONS_OUTPUT=console,json=perf_regions.json

export LD_LIBRARY_PATH="$PWD/build/build_papi_mpi_fort:$LD_LIBRARY_PATH"
./my_instrumented_application
```

Use `papi_avail` and `papi_native_avail` to discover available PAPI events on the
current platform.

To let PerfRegions detect the memory-channel events automatically:

```bash
export PERF_REGIONS_COUNTERS=PR_BANDWIDTH_BYTES,PR_BANDWIDTH_GB,PR_MEM_TRANSFER_BYTES,PR_MEM_TRANSFER_GB
./my_instrumented_application
```

The individual detected channel counters are included in the output together
with all four consolidated memory-transfer values.

## Testing

The canonical test entry point is:

```bash
python3 tests/run_tests.py
```

Useful variants:

```bash
make test                  # fast default suite
make test_papi             # PAPI-enabled core tests
make test_hardware         # PAPI hardware tests
make test_all              # full compile/runtime matrix
python3 tests/run_tests.py --matrix all --papi local
python3 tests/run_tests.py --cc clang --no-mpi --papi none
python3 tests/run_tests.py --fast
```

See the [testing guide](docs/source/developer_guide/testing.rst) and
[`tests/README.md`](tests/README.md) for the test-only local PAPI installation
and the complete list of options.

## License

See [`LICENSE`](LICENSE).
