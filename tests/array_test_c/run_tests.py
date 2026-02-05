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
            self.env["PERF_REGIONS_COUNTERS"] = (
                "PAPI_L1_TCM,PAPI_L2_TCM,PAPI_L3_TCM,WALLCLOCKTIME"
            )

        # Ensure we start from a clean state
        self.execute("make clean")

    def execute(self, command):
        subprocess.check_call(command.split(), cwd=self.base_dir, env=self.env)

    def test_01_original_execution(self):
        """Test compiling and running the original un-instrumented code."""
        print("\nRunning original execution test...")
        self.execute("make build/array_test")
        print("Running ./build/array_test")
        self.execute("taskset -c 0 build/array_test")

    def test_02_instrumented_execution(self):
        """Test compiling and running the instrumented code."""
        print("\nRunning instrumented execution test...")
        self.execute("make build/array_test_perf_regions")
        print("Running ./build/array_test_perf_regions")
        self.execute("taskset -c 0 build/array_test_perf_regions")

    def test_03_instrumentation_diffs(self):
        """Test that the instrumentation matches reference and src is untouched."""
        print("\nRunning instrumentation diff check...")
        # Build instrumented version to trigger instrumentation
        self.execute("make build/array_test_perf_regions")

        # Verify src matches ORIG (should not change)
        print("Verifying src has NOT changed...")
        self.execute("diff src/array_test.c test_data/array_test.c_TEST_ORIG")

        # Verify build output matches reference PR
        print("Verifying build output matches REFERENCE PR...")
        self.execute("diff build/array_test.c test_data/array_test.c_TEST_PR")

    def test_04_cleanup_restoration(self):
        """Test that make clean removes artifacts."""
        print("\nRunning cleanup test...")
        # Build first
        self.execute("make build/array_test_perf_regions")

        # Then clean
        print("Cleaning up...")
        self.execute("make clean")

        # Verify build dir is gone or empty
        # We can just check if file exists
        if os.path.exists(os.path.join(self.base_dir, "build/array_test.c")):
             pass # Fail? Or maybe make clean doesn't remove dir, just files. 
                  # Makefile says rm -rf build
        
        self.assertFalse(os.path.exists(os.path.join(self.base_dir, "build/array_test.c")), 
                         "build/array_test.c should actully be removed")

        # Verify src is still intact
        self.execute("diff src/array_test.c test_data/array_test.c_TEST_ORIG")


if __name__ == "__main__":
    unittest.main()
