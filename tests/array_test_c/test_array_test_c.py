import pytest
import os
import subprocess


@pytest.fixture
def base_dir():
    return os.path.dirname(os.path.abspath(__file__))


@pytest.fixture
def env(request, base_dir):
    env_vars = os.environ.copy()

    # LD_LIBRARY_PATH
    ld_lib_path = os.path.abspath(os.path.join(base_dir, "../../build"))
    existing_ld = env_vars.get("LD_LIBRARY_PATH", "")
    env_vars["LD_LIBRARY_PATH"] = f"{ld_lib_path}:{existing_ld}"

    # Output file
    env_vars["PERF_REGIONS_OUTPUT"] = "json=output_perf_regions.json"

    # Mode
    # If using 'indirect=True' with parametrize, request.param will be available
    # Default to 'papi' if not parametrized or not set
    mode = getattr(request, "param", "papi")

    if mode == "wallclock":
        env_vars["PERF_REGIONS_COUNTERS"] = "WALLCLOCKTIME"
    else:
        # PAPI default - Only set if not already in env (though checking logic here)
        if "PERF_REGIONS_COUNTERS" not in env_vars:
            if env_vars.get("USE_PAPI") == "0":
                env_vars["PERF_REGIONS_COUNTERS"] = "WALLCLOCKTIME"
            else:
                env_vars["PERF_REGIONS_COUNTERS"] = (
                    "PAPI_L1_TCM,PAPI_L2_TCM,PAPI_L3_TCM,WALLCLOCKTIME"
                )

    return env_vars


@pytest.fixture
def clean_artifact(base_dir, env):
    subprocess.check_call(["make", "clean"], cwd=base_dir, env=env)
    output_json = os.path.join(base_dir, "output_perf_regions.json")
    if os.path.exists(output_json):
        os.remove(output_json)

    yield

    subprocess.check_call(["make", "clean"], cwd=base_dir, env=env)
    if os.path.exists(output_json):
        os.remove(output_json)


def run_command(cmd, cwd, env):
    # Split command string into list if string
    if isinstance(cmd, str):
        cmd = cmd.split()
    subprocess.check_call(cmd, cwd=cwd, env=env)


@pytest.mark.parametrize("env", ["papi", "wallclock"], indirect=True)
def test_original_execution(base_dir, env, clean_artifact):
    run_command("make build/array_test", base_dir, env)
    run_command("taskset -c 0 build/array_test", base_dir, env)


@pytest.mark.parametrize("env", ["papi", "wallclock"], indirect=True)
def test_instrumented_execution(base_dir, env, clean_artifact):
    run_command("make build/array_test_perf_regions", base_dir, env)
    run_command("taskset -c 0 build/array_test_perf_regions", base_dir, env)

    # Check if output created
    assert os.path.exists(os.path.join(base_dir, "output_perf_regions.json"))


def test_instrumentation_diffs(base_dir, env, clean_artifact):
    # Uses default env (papi)
    run_command("make build/array_test_perf_regions", base_dir, env)

    # Check src untouched
    run_command("diff src/array_test.c test_data/array_test.c_TEST_ORIG", base_dir, env)

    # Check build instrumented
    run_command("diff build/array_test.c test_data/array_test.c_TEST_PR", base_dir, env)


def test_cleanup_restoration(base_dir, env, clean_artifact):
    # 1. Create instrumented file in build/
    run_command("make build/array_test_perf_regions", base_dir, env)

    # 2. Run cleanup script on build/array_test.c
    script = os.path.join(base_dir, "bin/perf_regions_instrumentation.py")
    target = os.path.join(base_dir, "build/array_test.c")

    # Need to run python script.
    run_command(f"python3 {script} {target} cleanup", base_dir, env)

    # 3. Diff with ORIG
    run_command(f"diff {target} test_data/array_test.c_TEST_ORIG", base_dir, env)
