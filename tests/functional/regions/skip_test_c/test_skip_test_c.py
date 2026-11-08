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
