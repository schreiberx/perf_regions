import unittest
import subprocess
import os


class TestArrayTest(unittest.TestCase):
    def setUp(self):
        # Current file is in tests/array_test_c/
        self.base_dir = os.path.dirname(os.path.abspath(__file__))

        # Prepare environment
        self.env = os.environ.copy()

        # Add ../../src to LD_LIBRARY_PATH
        src_path = os.path.abspath(os.path.join(self.base_dir, "../../src"))
        existing_ld = self.env.get("LD_LIBRARY_PATH", "")
        self.env["LD_LIBRARY_PATH"] = f"{src_path}:{existing_ld}"

        # Set PERF_REGIONS_COUNTERS if not already set
        if "PERF_REGIONS_COUNTERS" not in self.env:
            self.env["PERF_REGIONS_COUNTERS"] = "PAPI_L1_TCM,PAPI_L2_TCM,PAPI_L3_TCM,WALLCLOCKTIME"

        # Ensure we start from a clean state
        subprocess.check_call(["make", "clean"], cwd=self.base_dir, env=self.env)

    def test_01_original_execution(self):
        """Test compiling and running the original un-instrumented code."""
        print("\nRunning original execution test...")
        subprocess.check_call(["make", "array_test"], cwd=self.base_dir, env=self.env)
        print("Running ./array_test")
        subprocess.check_call(["taskset", "-c", "0", "./array_test"], cwd=self.base_dir, env=self.env)

    def test_02_instrumented_execution(self):
        """Test compiling and running the instrumented code."""
        print("\nRunning instrumented execution test...")
        subprocess.check_call(["make", "array_test_perf_regions"], cwd=self.base_dir, env=self.env)
        print("Running ./array_test_perf_regions")
        subprocess.check_call(["taskset", "-c", "0", "./array_test_perf_regions"], cwd=self.base_dir, env=self.env)

    def test_03_instrumentation_diffs(self):
        """Test that the instrumentation modifies the source code correctly."""
        print("\nRunning instrumentation diff check...")
        # Build instrumented version to trigger instrumentation
        subprocess.check_call(["make", "array_test_perf_regions"], cwd=self.base_dir, env=self.env)

        # Verify instrumentation changed the file
        print("Verifying instrumentation changed the source...")
        try:
            subprocess.check_call(
                ["diff", "array_test.c", "array_test.c_TEST_ORIG"],
                cwd=self.base_dir,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            self.fail("array_test.c should be different from array_test.c_TEST_ORIG after instrumentation")
        except subprocess.CalledProcessError:
            # diff returns 1 if files differ, which is what we want
            pass

        # Verify it matches the PR reference
        print("Verifying match with REFERENCE PR...")
        subprocess.check_call(["diff", "array_test.c", "array_test.c_TEST_PR"], cwd=self.base_dir, env=self.env)

    def test_04_cleanup_restoration(self):
        """Test that make clean restores the original source code."""
        print("\nRunning cleanup restoration test...")
        # Build first to modify the file
        subprocess.check_call(["make", "array_test_perf_regions"], cwd=self.base_dir, env=self.env)

        # Then clean
        print("Cleaning up...")
        subprocess.check_call(["make", "clean"], cwd=self.base_dir, env=self.env)

        # Verify restoration
        print("Verifying restoration...")
        subprocess.check_call(["diff", "array_test.c", "array_test.c_TEST_ORIG"], cwd=self.base_dir, env=self.env)


if __name__ == "__main__":
    unittest.main()
