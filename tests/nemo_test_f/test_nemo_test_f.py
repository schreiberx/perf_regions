import sys


def test_original_execution(base_dir, env, run_command, cpu_affinity_prefix):
    run_command(["make", "build/nemo_test"], cwd=base_dir, env=env)
    run_command([*cpu_affinity_prefix, "build/nemo_test"], cwd=base_dir, env=env)


def test_instrumented_execution(
    base_dir, env, run_command, cpu_affinity_prefix, output_json, read_json
):
    run_command(["make", "build/nemo_test_perf_regions"], cwd=base_dir, env=env)
    run_command(
        [*cpu_affinity_prefix, "build/nemo_test_perf_regions"],
        cwd=base_dir,
        env=env,
    )

    assert output_json.exists(), "JSON output not created"
    assert isinstance(read_json(output_json), list)


def test_instrumentation_diffs(base_dir, env, run_command):
    run_command(["make", "build/nemo_test_perf_regions"], cwd=base_dir, env=env)

    run_command(
        ["diff", "src/nemo_test.F90", "test_data/nemo_test.F90_TEST_ORIG"],
        cwd=base_dir,
        env=env,
    )
    run_command(
        ["diff", "build/nemo_test.F90", "test_data/nemo_test.F90_TEST_PR"],
        cwd=base_dir,
        env=env,
    )


def test_cleanup_restoration(base_dir, env, run_command, instrument_script):
    run_command(["make", "build/nemo_test_perf_regions"], cwd=base_dir, env=env)

    target = base_dir / "build" / "nemo_test.F90"
    run_command(
        [sys.executable, str(instrument_script), "cleanup", str(target)],
        cwd=base_dir,
        env=env,
    )
    run_command(
        ["diff", str(target), "test_data/nemo_test.F90_TEST_ORIG"],
        cwd=base_dir,
        env=env,
    )
