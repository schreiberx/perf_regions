import pytest

def find_region(data, name):
    return next((region for region in data if region["region"] == name), None)


def assert_nested_region_is_shorter(inner, outer):
    if "wallclock_time" in inner and "wallclock_time" in outer:
        assert inner["wallclock_time"] < outer["wallclock_time"]


def test_skip_execution(base_dir, env, run_command, output_json, read_json):
    run_command(["make", "build/skip_test"], cwd=base_dir, env=env)

    run_env = env.copy()
    run_env["PERF_REGIONS_SKIP_N"] = "2"
    run_env["PERF_REGIONS_OUTPUT"] = f"json={output_json}"

    run_command(["./build/skip_test"], cwd=base_dir, env=run_env)

    assert output_json.exists(), "JSON output not created"
    data = read_json(output_json)

    outer = find_region(data, "Outer")
    inner = find_region(data, "Inner")
    independent = find_region(data, "Independent")
    independent_inner = find_region(data, "IndependentInner")

    assert outer is not None
    assert outer["counter"] == 3
    assert outer["skipped"] == 2

    assert inner is not None
    assert_nested_region_is_shorter(inner, outer)

    assert independent is not None
    assert independent["counter"] == 3
    assert independent["skipped"] == 2

    assert independent_inner is not None
    assert independent_inner["counter"] == 15
    assert independent_inner["skipped"] == 10
    assert_nested_region_is_shorter(independent_inner, independent)


def test_no_skip_execution(base_dir, env, run_command, tmp_path, read_json):
    run_command(["make", "build/skip_test"], cwd=base_dir, env=env)

    output_json = tmp_path / "output_perf_regions_full.json"
    run_env = env.copy()
    run_env["PERF_REGIONS_SKIP_N"] = "0"
    run_env["PERF_REGIONS_OUTPUT"] = f"json={output_json}"

    run_command(["./build/skip_test"], cwd=base_dir, env=run_env)

    data = read_json(output_json)
    outer = find_region(data, "Outer")
    assert outer is not None
    assert outer["counter"] == 5
    assert outer["skipped"] == 0


@pytest.mark.papi_hardware
@pytest.mark.parametrize(
    "pseudo_event",
    [
        "PR_BANDWIDTH_BYTES",
        "PR_MEM_TRANSFER_GB",
    ],
)
def test_memory_transfer_pseudo_events(
    base_dir,
    env,
    run_command,
    output_json,
    read_json,
    papi_test_skip_reason,
    pseudo_event,
):
    if papi_test_skip_reason:
        pytest.skip(papi_test_skip_reason)

    run_command(["make", "build/skip_test"], cwd=base_dir, env=env)

    run_env = env.copy()
    run_env["PERF_REGIONS_COUNTERS"] = f"{pseudo_event},WALLCLOCKTIME"
    run_env["PERF_REGIONS_BANDWIDTH_EVENTS"] = "PAPI_TOT_CYC"
    run_env["PERF_REGIONS_BANDWIDTH_BYTES_PER_COUNT"] = "64"
    run_env["PERF_REGIONS_OUTPUT"] = f"json={output_json}"

    run_command(["./build/skip_test"], cwd=base_dir, env=run_env)

    data = read_json(output_json)
    region = find_region(data, "Outer")
    assert region is not None

    counters = region["counters"]
    assert "PAPI_TOT_CYC" in counters
    for name in (
        "PR_BANDWIDTH_BYTES",
        "PR_BANDWIDTH_GB",
        "PR_MEM_TRANSFER_BYTES",
        "PR_MEM_TRANSFER_GB",
    ):
        assert name in counters

    expected_bytes = counters["PAPI_TOT_CYC"] * 64.0
    assert counters["PR_BANDWIDTH_BYTES"] == pytest.approx(expected_bytes)
    assert counters["PR_MEM_TRANSFER_BYTES"] == pytest.approx(expected_bytes)
    assert counters["PR_BANDWIDTH_GB"] == pytest.approx(expected_bytes / 1e9)
    assert counters["PR_MEM_TRANSFER_GB"] == pytest.approx(expected_bytes / 1e9)


@pytest.mark.papi_hardware
def test_papi_counter_through_perf_regions(
    base_dir,
    env,
    run_command,
    output_json,
    read_json,
    papi_test_skip_reason,
    papi_counters,
):
    if papi_test_skip_reason:
        pytest.skip(papi_test_skip_reason)

    requested_counters = [
        counter.strip()
        for counter in papi_counters.split(",")
        if counter.strip() and counter.strip() != "WALLCLOCKTIME"
    ]
    if not requested_counters:
        pytest.skip("No PAPI counters were requested")

    run_command(["make", "build/skip_test"], cwd=base_dir, env=env)

    run_env = env.copy()
    run_env["PERF_REGIONS_COUNTERS"] = papi_counters
    run_env["PERF_REGIONS_OUTPUT"] = f"json={output_json}"

    run_command(["./build/skip_test"], cwd=base_dir, env=run_env)

    data = read_json(output_json)
    region = find_region(data, "Outer")
    assert region is not None
    counters = region["counters"]

    for counter in requested_counters:
        assert counter in counters, (
            f"Requested counter {counter} is missing from PerfRegions output"
        )
        assert counters[counter] > 0, (
            f"Requested counter {counter} was not incremented"
        )
