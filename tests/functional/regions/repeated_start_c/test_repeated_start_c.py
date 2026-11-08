def test_first_region_name_is_retained(
    base_dir, env, run_command, output_json, read_json
):
    run_command(["make", "build/repeated_start"], cwd=base_dir, env=env)
    run_command(["build/repeated_start"], cwd=base_dir, env=env)

    data = read_json(output_json)
    assert [region["region"] for region in data] == ["first"]
