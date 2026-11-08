import pytest


@pytest.mark.papi_hardware
def test_monitor_run(
    base_dir, env, run_command, papi_test_skip_reason
):
    if papi_test_skip_reason:
        pytest.skip(papi_test_skip_reason)

    run_env = env.copy()
    counters = [
        event
        for event in run_env.get("PERF_REGIONS_COUNTERS", "PAPI_TOT_CYC").split(",")
        if event and event != "WALLCLOCKTIME"
    ]
    run_env["PERF_REGIONS_COUNTERS"] = ",".join(counters) or "PAPI_TOT_CYC"

    run_command(["make"], cwd=base_dir, env=run_env)
    run_command(
        ["./bandwidth_monitor_papi", "-i", "0.1", "-t", "0.2"],
        cwd=base_dir,
        env=run_env,
    )
