import unittest
import subprocess
import os


class TestNemoTestDirect(unittest.TestCase):
    def setUp(self):
        self.base_dir = os.path.dirname(os.path.abspath(__file__))
        self.env = os.environ.copy()

        src_path = os.path.abspath(os.path.join(self.base_dir, "../../src"))
        existing_ld = self.env.get("LD_LIBRARY_PATH", "")
        self.env["LD_LIBRARY_PATH"] = f"{src_path}:{existing_ld}"

        # Initial counters
        if "PERF_REGIONS_COUNTERS" not in self.env:
            self.env["PERF_REGIONS_COUNTERS"] = "PAPI_L3_TCM,WALLCLOCKTIME"

        self.execute("make clean")

    def execute(self, command):
        subprocess.check_call(command.split(), cwd=self.base_dir, env=self.env)

    def test_variable_sizes(self):
        # Build
        print("Building nemo_test_perf_regions")
        self.execute("make")

        sizes = [1, 2, 4, 8, 16, 32, 64, 128]
        for i in sizes:
            print(f"\n***********************************")
            print(f"SIZE: {i}")
            print(f"***********************************")

            run_env = self.env.copy()
            run_env["JPI"] = str(i)
            run_env["JPJ"] = str(i)
            run_env["JPK"] = "72"
            run_env["PERF_REGIONS_COUNTERS"] = "PAPI_L3_TCM,WALLCLOCKTIME"

            subprocess.check_call(
                ["taskset", "-c", "0", "build/nemo_test_perf_regions"],
                cwd=self.base_dir,
                env=run_env,
            )
        
        self.execute("make clean")


if __name__ == "__main__":
    unittest.main()
