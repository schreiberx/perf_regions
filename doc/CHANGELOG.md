

# 2025-02-05

- Started changelog

- Using `pytest` for test cases.

- All test cases have been cleaned up:
    - `src`: source code
    - `bin`: scripts
    - `build`: for building process
    - `test_data`: Verification for changes with perf_region
    - `pytests.py`: All test cases for this test

- `perf_region` Libraries are now postfixed with `_mpi` or `_papi`.
  E.g., enabling a build with MPI and without PAPI would result in `perf_region_mpi.so`.
