def test_region_table_grows_as_needed(
    base_dir, env, run_command, output_json, read_json
):
    run_command(["make", "build/dynamic_regions"], cwd=base_dir, env=env)
    run_command(["build/dynamic_regions"], cwd=base_dir, env=env)

    data = read_json(output_json)
    assert [region["region"] for region in data] == ["outer", "inner", "late"]
