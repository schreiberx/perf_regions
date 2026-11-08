import pytest


@pytest.mark.slow
def test_stream_execution(base_dir, env, run_command, output_json, read_json):
    run_command(["make", "build/stream_c_perfregions"], cwd=base_dir, env=env)
    run_command(["./build/stream_c_perfregions"], cwd=base_dir, env=env)

    assert output_json.exists(), "JSON output not created"
    data = read_json(output_json)
    assert isinstance(data, list)
    assert len(data) > 0
