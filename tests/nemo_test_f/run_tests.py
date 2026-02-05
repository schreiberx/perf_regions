import unittest
import subprocess
import os


class TestNemoTestFortran(unittest.TestCase):
    def setUp(self):
        self.base_dir = os.path.dirname(os.path.abspath(__file__))
        self.env = os.environ.copy()

        src_path = os.path.abspath(os.path.join(self.base_dir, "../../src"))
        existing_ld = self.env.get("LD_LIBRARY_PATH", "")
        self.env["LD_LIBRARY_PATH"] = f"{src_path}:{existing_ld}"

        if "PERF_REGIONS_COUNTERS" not in self.env:
            self.env["PERF_REGIONS_COUNTERS"] = (
                "PAPI_L1_TCM,PAPI_L2_TCM,PAPI_L3_TCM,WALLCLOCKTIME"
            )

        self.execute("make clean")

    def execute(self, command):
        subprocess.check_call(command.split(), cwd=self.base_dir, env=self.env)

    def test_01_original_execution(self):
        print("\nRunning original execution test...")
        self.execute("make build/nemo_test")
        self.execute("taskset -c 0 build/nemo_test")

    def test_02_instrumented_execution(self):
        print("\nRunning instrumented execution test...")
        self.execute("make build/nemo_test_perf_regions")
        self.execute("taskset -c 0 build/nemo_test_perf_regions")

    def test_03_instrumentation_diffs(self):
        print("\nRunning instrumentation diff check...")
        self.execute("make build/nemo_test_perf_regions")

        # 1. Verify src is UNTOUCHED
        self.execute("diff src/nemo_test.F90 test_data/nemo_test.F90_TEST_ORIG")

        # 2. Verify build output MATCHES PR reference
        self.execute("diff build/nemo_test.F90 test_data/nemo_test.F90_TEST_PR")

    def test_04_cleanup(self):
        print("\nRunning cleanup test...")
        self.execute("make build/nemo_test_perf_regions")
        self.execute("make clean")
        self.assertFalse(os.path.exists(os.path.join(self.base_dir, "build/nemo_test.F90")))
        # Src should still be untouched
        self.execute("diff src/nemo_test.F90 test_data/nemo_test.F90_TEST_ORIG")


if __name__ == "__main__":
    unittest.main()
