# Tests for PerfRegions

The canonical test entry point is:

```bash
python3 tests/run_tests.py
```

The full testing documentation, including MPI/PAPI variants, the test-only
local PAPI installation, pytest usage, and the instrumentation golden files, is
maintained in the Sphinx documentation:

- `docs/source/developer_guide/testing.rst`
- `tests/tools/papi-local/README.md`

Quick example:

```bash
python3 tests/run_tests.py --venv
python3 tests/run_tests.py --mpi --papi local
```

Run the complete PAPI/wallclock matrix with:

```bash
tests/run_tests_all.sh
```
