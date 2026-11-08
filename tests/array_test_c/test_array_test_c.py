import sys


def test_original_execution(base_dir, env, run_command, cpu_affinity_prefix):
    run_command(["make", "build/array_test"], cwd=base_dir, env=env)
    run_command([*cpu_affinity_prefix, "build/array_test"], cwd=base_dir, env=env)


def test_instrumented_execution(
    base_dir, env, run_command, cpu_affinity_prefix, output_json, read_json
):
    run_command(["make", "build/array_test_perf_regions"], cwd=base_dir, env=env)
    run_command(
        [*cpu_affinity_prefix, "build/array_test_perf_regions"],
        cwd=base_dir,
        env=env,
    )

    assert output_json.exists(), "JSON output not created"
    assert isinstance(read_json(output_json), list)


def test_instrumentation_diffs(base_dir, env, run_command):
    run_command(["make", "build/array_test_perf_regions"], cwd=base_dir, env=env)

    run_command(
        ["diff", "src/array_test.c", "test_data/array_test.c_TEST_ORIG"],
        cwd=base_dir,
        env=env,
    )
    run_command(
        ["diff", "build/array_test.c", "test_data/array_test.c_TEST_PR"],
        cwd=base_dir,
        env=env,
    )


def test_cleanup_restoration(base_dir, env, run_command, instrument_script):
    run_command(["make", "build/array_test_perf_regions"], cwd=base_dir, env=env)

    target = base_dir / "build" / "array_test.c"
    run_command(
        [sys.executable, str(instrument_script), "cleanup", str(target)],
        cwd=base_dir,
        env=env,
    )
    run_command(
        ["diff", str(target), "test_data/array_test.c_TEST_ORIG"],
        cwd=base_dir,
        env=env,
    )
