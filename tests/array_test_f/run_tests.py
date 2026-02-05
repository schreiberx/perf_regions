import unittest
import subprocess
import os

class TestArrayTestFortran(unittest.TestCase):
    def setUp(self):
        self.base_dir = os.path.dirname(os.path.abspath(__file__))
        self.env = os.environ.copy()
        
        src_path = os.path.abspath(os.path.join(self.base_dir, "../../src"))
        existing_ld = self.env.get("LD_LIBRARY_PATH", "")
        self.env["LD_LIBRARY_PATH"] = f"{src_path}:{existing_ld}"

        if "PERF_REGIONS_COUNTERS" not in self.env:
            self.env["PERF_REGIONS_COUNTERS"] = "PAPI_L1_TCM,PAPI_L2_TCM,PAPI_L3_TCM,WALLCLOCKTIME"

        subprocess.check_call(["make", "clean"], cwd=self.base_dir, env=self.env)

    def test_workflow(self):
        # make tests
        subprocess.check_call(["make", "tests"], cwd=self.base_dir, env=self.env)
        
        # Run executed binaries (original and instrumented)
        print("Running ./array_test")
        subprocess.check_call(["taskset", "-c", "0", "./array_test"], cwd=self.base_dir, env=self.env)
        
        print("Running ./array_test_perf_regions")
        subprocess.check_call(["taskset", "-c", "0", "./array_test_perf_regions"], cwd=self.base_dir, env=self.env)
        
        # Verify changes
        # diff array_test.F90 array_test.F90_TEST_ORIG > /dev/null
        # if [ 1 -eq 0 ]; then echo "This should have failed"; exit 1; fi
        try:
             subprocess.check_call(["diff", "array_test.F90", "array_test.F90_TEST_ORIG"], cwd=self.base_dir, stdout=subprocess.DEVNULL)
             self.fail("array_test.F90 should be different from array_test.F90_TEST_ORIG")
        except subprocess.CalledProcessError:
             pass 

        # diff array_test.F90 array_test.F90_TEST_PR
        subprocess.check_call(["diff", "array_test.F90", "array_test.F90_TEST_PR"], cwd=self.base_dir)
        
        # Cleanup and verify restore
        subprocess.check_call(["make", "clean"], cwd=self.base_dir, env=self.env)
        
        subprocess.check_call(["diff", "array_test.F90", "array_test.F90_TEST_ORIG"], cwd=self.base_dir)

if __name__ == '__main__':
    unittest.main()
