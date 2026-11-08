import pytest


def find_region(data, name):
    return next((region for region in data if region["region"] == name), None)


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

    run_command(["make", "build/papi_probe"], cwd=base_dir, env=env)

    run_env = env.copy()
    run_env["PERF_REGIONS_COUNTERS"] = f"{pseudo_event},WALLCLOCKTIME"
    run_env["PERF_REGIONS_BANDWIDTH_EVENTS"] = "PAPI_TOT_CYC"
    run_env["PERF_REGIONS_BANDWIDTH_BYTES_PER_COUNT"] = "64"
    run_env["PERF_REGIONS_OUTPUT"] = f"json={output_json}"

    run_command(["./build/papi_probe"], cwd=base_dir, env=run_env)

    data = read_json(output_json)
    region = find_region(data, "papi_probe")
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

    run_command(["make", "build/papi_probe"], cwd=base_dir, env=env)

    run_env = env.copy()
    run_env["PERF_REGIONS_COUNTERS"] = papi_counters
    run_env["PERF_REGIONS_OUTPUT"] = f"json={output_json}"

    run_command(["./build/papi_probe"], cwd=base_dir, env=run_env)

    data = read_json(output_json)
    region = find_region(data, "papi_probe")
    assert region is not None
    counters = region["counters"]

    for counter in requested_counters:
        assert counter in counters, (
            f"Requested counter {counter} is missing from PerfRegions output"
        )
        assert counters[counter] > 0, (
            f"Requested counter {counter} was not incremented"
        )
