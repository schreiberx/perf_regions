# Local PAPI for tests

This directory contains **test-only** helpers that build and install PAPI into
a repo-local prefix. They do not use `sudo`, do not install into system
directories, and do not modify shell startup files.

The installers install the PAPI user-space library and tools. Hardware counter
access is still controlled by the kernel (`perf_event_paranoid`, container
permissions, VM support, etc.).

## Install

```bash
tests/tools/papi-local/install.sh
```

By default this installs PAPI `7.2.0` under:

```text
tests/.deps/papi/7.2.0-<arch>-<compiler>/
```

and updates:

```text
tests/.deps/papi/current -> <installed-prefix>
```

The downloaded tarball is cached in `tests/.deps/cache/`.

To use an existing tarball:

```bash
tests/tools/papi-local/install.sh --tarball /path/to/papi-7.2.0.tar.gz
```

The installer adds a harmless `-Wno-error=discarded-qualifiers` build flag by
default because PAPI 7.2.0's bundled libpfm4 is not warning-clean with newer
GCC versions. Override `PAPI_BUILD_CFLAGS` if you need different build flags.

To install another version, pass `--version` and preferably an explicit
checksum:

```bash
tests/tools/papi-local/install.sh \
    --version 7.1.0 \
    --url https://example.invalid/papi-7.1.0.tar.gz \
    --sha256 <sha256>
```

## Activate

```bash
source tests/tools/papi-local/env.sh
```

The canonical test runner can also do this automatically:

```bash
python3 tests/run_tests.py --mpi --papi local
```

This exports, among others:

```text
PAPI_PREFIX
PAPI_CFLAGS
PAPI_LDFLAGS
PATH
LD_LIBRARY_PATH
PKG_CONFIG_PATH
PERF_REGIONS_TEST_PAPI=1
```

It also sets a conservative default if `PERF_REGIONS_COUNTERS` is unset:

```text
PERF_REGIONS_COUNTERS=PAPI_TOT_CYC,WALLCLOCKTIME
```

## Configure, build, and test

```bash
source tests/tools/papi-local/env.sh

./configure --with-papi="$PAPI_PREFIX"
make -j
make run_tests
```

Because `env.sh` exports `PAPI_CFLAGS` and `PAPI_LDFLAGS`, a plain
`./configure` should also detect the local installation.

## Validate

```bash
tests/tools/papi-local/doctor.sh
```

The doctor checks:

- local headers, library, and PAPI tools,
- compile/link against the local PAPI,
- basic `PAPI_TOT_CYC` access,
- `papi_avail`/`papi_native_avail`,
- `perf_event_paranoid`.

Use `--strict` to treat unavailable hardware counters as an error.

On many containers or VMs hardware events are unavailable. In that case, run
tests in wallclock-only mode:

```bash
export PERF_REGIONS_COUNTERS=WALLCLOCKTIME
```

or let the test harness fall back if supported.

## Cleanup

Remove the local installation and build tree:

```bash
tests/tools/papi-local/install.sh --clean
```

Also remove the downloaded tarball cache:

```bash
tests/tools/papi-local/install.sh --clean --clean-cache
```

## Safety

- The default prefix is under `tests/.deps/`.
- `install.sh` refuses prefixes outside `tests/.deps/` unless
  `--allow-external-prefix` is given.
- `install.sh` refuses to run as root unless `--allow-root` is given.
- The scripts never change kernel settings; they only report them.
- `tests/.deps/` is gitignored.
