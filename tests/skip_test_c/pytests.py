import pytest
import os
import subprocess
import json


@pytest.fixture
def base_dir():
    return os.path.dirname(os.path.abspath(__file__))


@pytest.fixture
def env(request, base_dir):
    env_vars = os.environ.copy()
    ld_lib_path = os.path.abspath(os.path.join(base_dir, "../../build"))
    existing_ld = env_vars.get("LD_LIBRARY_PATH", "")
    env_vars["LD_LIBRARY_PATH"] = f"{ld_lib_path}:{existing_ld}"

    # Mode
    mode = getattr(request, "param", "papi")
    if mode == "wallclock":
        env_vars["PERF_REGIONS_COUNTERS"] = "WALLCLOCKTIME"
    else:
        # PAPI default
        if "PERF_REGIONS_COUNTERS" not in env_vars:
            if env_vars.get("USE_PAPI") == "0":
                env_vars["PERF_REGIONS_COUNTERS"] = "WALLCLOCKTIME"
            else:
                env_vars["PERF_REGIONS_COUNTERS"] = "PAPI_L1_TCM,WALLCLOCKTIME"

    return env_vars


@pytest.fixture(autouse=True)
def clean_up(base_dir, env):
    subprocess.check_call(["make", "clean"], cwd=base_dir, env=env)
    for f in ["output_perf_regions.json", "output_perf_regions_full.json"]:
        path = os.path.join(base_dir, f)
        if os.path.exists(path):
            os.remove(path)
    yield
    subprocess.check_call(["make", "clean"], cwd=base_dir, env=env)
    for f in ["output_perf_regions.json", "output_perf_regions_full.json"]:
        path = os.path.join(base_dir, f)
        if os.path.exists(path):
            os.remove(path)


@pytest.mark.parametrize("env", ["papi", "wallclock"], indirect=True)
def test_skip_execution(base_dir, env):
    subprocess.check_call(["make", "build/skip_test"], cwd=base_dir, env=env)

    run_env = env.copy()
    run_env["PERF_REGIONS_SKIP_N"] = "2"
    run_env["PERF_REGIONS_OUTPUT"] = "json=output_perf_regions.json"

    # Run
    # Using specific executable invocation
    cmd = ["./build/skip_test"]
    subprocess.check_call(cmd, cwd=base_dir, env=run_env)

    json_path = os.path.join(base_dir, "output_perf_regions.json")
    assert os.path.exists(json_path), "JSON output not created"

    with open(json_path, "r") as f:
        data = json.load(f)

    outer = next((r for r in data if r["region"] == "Outer"), None)
    inner = next((r for r in data if r["region"] == "Inner"), None)
    independent = next((r for r in data if r["region"] == "Independent"), None)
    independent_inner = next(
        (r for r in data if r["region"] == "IndependentInner"), None
    )

    assert outer is not None
    assert outer["counter"] == 3
    assert outer["skipped"] == 2

    inner["wallclock_time"] < outer["wallclock_time"]

    assert independent is not None
    assert independent["counter"] == 3
    assert independent["skipped"] == 2

    assert independent_inner is not None
    assert independent_inner["counter"] == 15
    assert independent_inner["skipped"] == 10

    independent_inner["wallclock_time"] < independent["wallclock_time"]


@pytest.mark.parametrize("env", ["papi", "wallclock"], indirect=True)
def test_no_skip_execution(base_dir, env):
    subprocess.check_call(["make", "build/skip_test"], cwd=base_dir, env=env)

    run_env = env.copy()
    run_env["PERF_REGIONS_SKIP_N"] = "0"
    run_env["PERF_REGIONS_OUTPUT"] = "json=output_perf_regions_full.json"

    cmd = ["./build/skip_test"]
    subprocess.check_call(cmd, cwd=base_dir, env=run_env)

    json_path = os.path.join(base_dir, "output_perf_regions_full.json")
    with open(json_path, "r") as f:
        data = json.load(f)

    outer = next((r for r in data if r["region"] == "Outer"), None)
    assert outer["counter"] == 5
    assert outer["skipped"] == 0
