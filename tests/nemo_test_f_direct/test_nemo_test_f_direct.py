import pytest


@pytest.mark.slow
def test_variable_sizes(base_dir, env, run_command, output_json):
    run_command(["make"], cwd=base_dir, env=env)

    sizes = [1, 2, 4, 8, 16, 32, 64, 128]
    for i in sizes:
        run_env = env.copy()
        run_env["JPI"] = str(i)
        run_env["JPJ"] = str(i)
        run_env["JPK"] = "72"

        run_command(["./build/nemo_test_perf_regions"], cwd=base_dir, env=run_env)

        assert output_json.exists(), f"Output not created for size {i}"
        output_json.unlink()
