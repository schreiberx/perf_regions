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

    env_vars["PERF_REGIONS_OUTPUT"] = "json=output_perf_regions.json"

    mode = getattr(request, "param", "papi")
    if mode == "wallclock":
        env_vars["PERF_REGIONS_COUNTERS"] = "WALLCLOCKTIME"
    else:
        # PAPI
        # Original: PAPI_L3_TCM,WALLCLOCKTIME
        if "PERF_REGIONS_COUNTERS" not in env_vars:
            if env_vars.get("USE_PAPI") == "0":
                env_vars["PERF_REGIONS_COUNTERS"] = "WALLCLOCKTIME"
            else:
                env_vars["PERF_REGIONS_COUNTERS"] = "PAPI_L3_TCM,WALLCLOCKTIME"

    return env_vars


@pytest.fixture(autouse=True)
def clean_artifact(base_dir, env):
    subprocess.check_call(["make", "clean"], cwd=base_dir, env=env)
    out = os.path.join(base_dir, "output_perf_regions.json")
    if os.path.exists(out):
        os.remove(out)
    yield
    subprocess.check_call(["make", "clean"], cwd=base_dir, env=env)
    if os.path.exists(out):
        os.remove(out)


@pytest.mark.parametrize("env", ["papi", "wallclock"], indirect=True)
def test_stream_execution(base_dir, env):
    subprocess.check_call(["make", "build/stream_c_perfregions"], cwd=base_dir, env=env)

    # Run
    cmd = ["./build/stream_c_perfregions"]
    subprocess.check_call(cmd, cwd=base_dir, env=env)

    out = os.path.join(base_dir, "output_perf_regions.json")
    assert os.path.exists(out), "JSON output not created"

    # Optional: Read JSON just to ensure it is valid
    with open(out, "r") as f:
        data = json.load(f)
    assert isinstance(data, list)
    assert len(data) > 0
    # Stream benchmark should have regions like "Copy", "Scale", "Add", "Triad" typically, or the ones defined in code
    # We just check we got data.
