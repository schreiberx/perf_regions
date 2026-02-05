import pytest
import os
import subprocess


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
        if env_vars.get("USE_PAPI") == "0":
            env_vars["PERF_REGIONS_COUNTERS"] = "WALLCLOCKTIME"
        else:
            env_vars["PERF_REGIONS_COUNTERS"] = "PAPI_L3_TCM,WALLCLOCKTIME"

    # Default output
    env_vars["PERF_REGIONS_OUTPUT"] = "json=output_perf_regions.json"
    return env_vars


@pytest.fixture(autouse=True)
def clean_up(base_dir, env):
    subprocess.check_call(["make", "clean"], cwd=base_dir, env=env)
    yield
    subprocess.check_call(["make", "clean"], cwd=base_dir, env=env)


@pytest.mark.parametrize("env", ["papi", "wallclock"], indirect=True)
def test_variable_sizes(base_dir, env):
    # Build
    subprocess.check_call(["make"], cwd=base_dir, env=env)

    sizes = [1, 2, 4, 8, 16, 32, 64, 128]
    for i in sizes:
        run_env = env.copy()
        run_env["JPI"] = str(i)
        run_env["JPJ"] = str(i)
        run_env["JPK"] = "72"

        # Execute
        # Note: shell=True is consistent with previous tests, but list requires ./
        cmd = ["./build/nemo_test_perf_regions"]
        subprocess.check_call(cmd, cwd=base_dir, env=run_env)

        # Check output
        output_path = os.path.join(base_dir, "output_perf_regions.json")
        assert os.path.exists(output_path), f"Output not created for size {i}"

        # Remove it for next iteration
        os.remove(output_path)
