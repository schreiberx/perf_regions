import os
import re
from pathlib import Path

import pytest


STREAM_REGIONS = ("copy", "scale", "add", "triad")
STREAM_TYPE_BYTES = 8  # STREAM is built with the default double type.


def find_region(data, name):
    return next((region for region in data if region["region"] == name), None)


def read_llc_bytes():
    for index in (3, 2):
        path = Path(f"/sys/devices/system/cpu/cpu0/cache/index{index}/size")
        try:
            text = path.read_text(encoding="utf-8").strip()
        except OSError:
            continue

        match = re.fullmatch(r"(\d+)([KMG]?)", text, re.IGNORECASE)
        if match is None:
            continue

        value = int(match.group(1))
        suffix = match.group(2).upper()
        multiplier = {"": 1, "K": 1024, "M": 1024**2, "G": 1024**3}[suffix]
        return value * multiplier

    return None


def stream_array_size():
    override = os.environ.get("PERF_REGIONS_TEST_STREAM_ARRAY_SIZE")
    if override:
        return int(override)

    llc_bytes = read_llc_bytes()
    if llc_bytes is None:
        return 8_000_000

    # Aim for four times the LLC per array, but keep the test bounded.
    desired = max(8_000_000, int(4 * llc_bytes / STREAM_TYPE_BYTES))
    return min(desired, 24_000_000)


def parse_stream_rates(output):
    rates = {}

    for line in output.splitlines():
        match = re.match(
            r"^(Copy|Scale|Add|Triad):\s+"
            r"([0-9]+(?:\.[0-9]+)?)\s+"
            r"([0-9]+(?:\.[0-9]+)?)",
            line,
        )
        if match is None:
            continue

        rates[match.group(1).lower()] = {
            "best": float(match.group(2)),
            "avg": float(match.group(3)),
        }

    return rates


def expected_region_bytes(region, array_size, ntimes):
    array_traversals = {
        "copy": 2,
        "scale": 2,
        "add": 3,
        "triad": 3,
        "total": 10,
    }
    return array_traversals[region] * STREAM_TYPE_BYTES * array_size * ntimes


def measured_bandwidth_gbps(region):
    wallclock = region["wallclock_time"]
    assert wallclock > 0.0
    return region["counters"]["PR_BANDWIDTH_BYTES"] / wallclock / 1e9


@pytest.mark.slow
def test_stream_execution(base_dir, env, run_command, output_json, read_json):
    run_command(["make", "build/stream_c_perfregions"], cwd=base_dir, env=env)
    run_command(["./build/stream_c_perfregions"], cwd=base_dir, env=env)

    assert output_json.exists(), "JSON output not created"
    data = read_json(output_json)
    assert isinstance(data, list)
    assert len(data) > 0


@pytest.mark.slow
@pytest.mark.papi_hardware
@pytest.mark.uncore
def test_stream_memory_bandwidth(
    base_dir,
    env,
    run_command,
    output_json,
    read_json,
    papi_test_skip_reason,
    cpu_affinity_prefix,
):
    if papi_test_skip_reason:
        pytest.skip(papi_test_skip_reason)

    array_size = stream_array_size()
    ntimes = max(2, int(os.environ.get("PERF_REGIONS_TEST_STREAM_NTIMES", "3")))

    run_command(
        [
            "make",
            "build/stream_c_perfregions",
            f"STREAM_ARRAY_SIZE={array_size}",
            f"STREAM_NTIMES={ntimes}",
        ],
        cwd=base_dir,
        env=env,
    )

    run_env = env.copy()
    run_env["PERF_REGIONS_COUNTERS"] = "PR_BANDWIDTH_BYTES,WALLCLOCKTIME"
    run_env["PERF_REGIONS_OUTPUT"] = f"json={output_json}"
    run_env["OMP_NUM_THREADS"] = "1"
    run_env["OMP_PROC_BIND"] = "close"

    if cpu_affinity_prefix:
        bandwidth_cpu = "0"
    else:
        try:
            bandwidth_cpu = str(min(os.sched_getaffinity(0)))
        except (AttributeError, OSError):
            bandwidth_cpu = None

    if bandwidth_cpu is not None:
        # Restrict uncore measurement to the socket running STREAM.
        run_env["PERF_REGIONS_BANDWIDTH_CPU"] = bandwidth_cpu

    result = run_command(
        [*cpu_affinity_prefix, "./build/stream_c_perfregions"],
        cwd=base_dir,
        env=run_env,
        check=False,
        capture_output=True,
    )

    if result.returncode != 0:
        output = result.stdout or ""
        if "Could not detect memory bandwidth events" in output:
            pytest.skip(
                "No supported uncore memory events are available "
                "(check perf_event_paranoid and PAPI uncore support)"
            )
        if "missing access rights" in output:
            pytest.skip("Uncore memory counters are not accessible")
        pytest.fail(f"STREAM run failed:\n{output}")

    assert output_json.exists(), "JSON output not created"
    data = read_json(output_json)
    stream_rates = parse_stream_rates(result.stdout or "")

    for name in STREAM_REGIONS:
        assert name in stream_rates, f"STREAM did not report a rate for {name}"

    for name in (*STREAM_REGIONS, "total"):
        region = find_region(data, name)
        assert region is not None, f"PerfRegions did not report region {name}"
        counters = region["counters"]

        for counter_name in (
            "PR_BANDWIDTH_BYTES",
            "PR_BANDWIDTH_GB",
            "PR_MEM_TRANSFER_BYTES",
            "PR_MEM_TRANSFER_GB",
        ):
            assert counter_name in counters, f"{name}: missing {counter_name}"

        bytes_value = counters["PR_BANDWIDTH_BYTES"]
        gb_value = counters["PR_BANDWIDTH_GB"]

        assert counters["PR_MEM_TRANSFER_BYTES"] == pytest.approx(
            bytes_value, rel=1e-12, abs=1e-12
        )
        assert counters["PR_MEM_TRANSFER_GB"] == pytest.approx(
            gb_value, rel=1e-12, abs=1e-12
        )
        assert gb_value == pytest.approx(bytes_value / 1e9, rel=1e-12, abs=1e-12)
        assert bytes_value > 0.0

        if name != "total":
            expected = expected_region_bytes(name, array_size, ntimes)
            assert bytes_value >= 0.65 * expected
            assert bytes_value <= 2.5 * expected

            stream_avg_gbps = stream_rates[name]["avg"]
            measured_gbps = measured_bandwidth_gbps(region)
            ratio = measured_gbps / stream_avg_gbps
            assert 0.6 <= ratio <= 2.5, (
                f"{name}: measured {measured_gbps:.3f} GB/s, "
                f"STREAM reported {stream_avg_gbps:.3f} GB/s"
            )

    total_region = find_region(data, "total")
    total_bytes = total_region["counters"]["PR_BANDWIDTH_BYTES"]
    expected_total = expected_region_bytes("total", array_size, ntimes)
    assert total_bytes >= 0.65 * expected_total
    assert total_bytes <= 2.5 * expected_total

    kernel_bytes = sum(
        find_region(data, name)["counters"]["PR_BANDWIDTH_BYTES"]
        for name in STREAM_REGIONS
    )
    assert total_bytes == pytest.approx(kernel_bytes, rel=0.2)
