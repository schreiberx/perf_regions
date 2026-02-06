import pytest
import os
import subprocess
import re


@pytest.fixture
def base_dir():
    return os.path.dirname(os.path.abspath(__file__))


@pytest.fixture
def env(request, base_dir):
    env_vars = os.environ.copy()
    ld_lib_path = os.path.abspath(os.path.join(base_dir, "../../build"))
    existing_ld = env_vars.get("LD_LIBRARY_PATH", "")
    env_vars["LD_LIBRARY_PATH"] = f"{ld_lib_path}:{existing_ld}"

    mode = getattr(request, "param", "papi")
    if env_vars.get("USE_PAPI") == "0":
        env_vars["PERF_REGIONS_COUNTERS"] = "WALLCLOCKTIME"
    elif mode == "wallclock":
        # bandwidth_monitor_papi uses PAPI directly and does not support WALLCLOCKTIME
        # Use a safe default event for testing the runner
        env_vars["PERF_REGIONS_COUNTERS"] = "PAPI_TOT_CYC"
    else:
        detected = False
        try:
            cmd = ["papi_native_avail", "-i", "UNC_M_CAS_COUNT", "--noqual"]
            output = subprocess.check_output(
                cmd, universal_newlines=True, stderr=subprocess.STDOUT
            )
            detected_imcs = []
            for line in output.splitlines():
                if "::UNC_M_CAS_COUNT" in line:
                    clean = line.replace("|", "").strip()
                    detected_imcs.append(clean)

            if detected_imcs:
                evt = detected_imcs[0].split("::")[0]
                prefix = re.sub(r"\d+$", "", evt)
                events = []
                for i in range(len(detected_imcs)):
                    base = f"{prefix}{i}::UNC_M_CAS_COUNT"
                    events.append(f"{base}.RD:cpu=0")
                    events.append(f"{base}.WR:cpu=0")
                env_vars["PERF_REGIONS_COUNTERS"] = ",".join(events)
                detected = True
        except Exception:
            pass

        if not detected:
            # Fallback: simple event
            env_vars["PERF_REGIONS_COUNTERS"] = "PAPI_TOT_CYC"

    return env_vars


@pytest.fixture(autouse=True)
def clean(base_dir):
    subprocess.check_call(["make", "clean"], cwd=base_dir)
    yield
    subprocess.check_call(["make", "clean"], cwd=base_dir)


@pytest.mark.parametrize("env", ["papi", "wallclock"], indirect=True)
def test_monitor_run(base_dir, env):
    env_vars = os.environ.copy()
    if env_vars.get("USE_PAPI") == "0":
        # Skip test if PAPI is not available, as the monitor relies on it directly and does not support a wallclock fallback
        return

    subprocess.check_call(["make"], cwd=base_dir, env=env)

    # Run with short timeout
    cmd = ["./bandwidth_monitor_papi", "-i", "0.1", "-t", "0.2"]
    subprocess.check_call(cmd, cwd=base_dir, env=env)
