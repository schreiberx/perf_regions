# Tests for PerfRegions

The canonical entry point is the Python test driver:

```bash
python3 tests/run_tests.py
```

Common make targets:

```bash
make test          # fast default suite
make test_papi     # PAPI-enabled core tests
make test_hardware # PAPI hardware tests
make test_all      # complete compile/runtime matrix
```

The complete compile/runtime matrix is also available through:

```bash
python3 tests/run_tests.py --matrix all --papi local
tests/run_tests_all.sh
```

## Layout

- `tests/functional/instrumentation/`: C, Fortran, and NEMO annotation tests.
- `tests/functional/regions/`: region skipping, repeated starts, and dynamic IDs.
- `tests/hardware/papi/`: direct PAPI and PerfRegions PAPI counter tests.
- `tests/benchmarks/stream_benchmark/`: the STREAM benchmark integration test.
- `tests/tools/papi-local/`: local test-only PAPI installation helpers.

The shared pytest fixtures live in `tests/conftest.py`. Build variants are kept
under `build/build_<features>/`, for example `build/build_papi_mpi_fort/`.

Full testing documentation is maintained in the Sphinx docs:

- `docs/source/developer_guide/testing.rst`
- `tests/tools/papi-local/README.md`
