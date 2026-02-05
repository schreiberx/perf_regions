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
            self.env["PERF_REGIONS_COUNTERS"] = "PAPI_L1_TCM,PAPI_L2_TCM,PAPI_L3_TCM,WALLCLOCKTIME"

        self.execute("make clean")

    def execute(self, command):
        subprocess.check_call(command.split(), cwd=self.base_dir, env=self.env)

    def test_workflow(self):
        # make tests
        self.execute("make tests")
        
        # Run
        print("Running ./nemo_test")
        self.execute("taskset -c 0 ./nemo_test")
        
        print("Running ./nemo_test_perf_regions")
        self.execute("taskset -c 0 ./nemo_test_perf_regions")
        
        # Verify changes
        try:
             subprocess.check_call(["diff", "nemo_test.F90", "nemo_test.F90_TEST_ORIG"], cwd=self.base_dir, stdout=subprocess.DEVNULL)
             self.fail("nemo_test.F90 should be different from TEST_ORIG")
        except subprocess.CalledProcessError:
             pass 

        # subprocess.check_call(["diff", "nemo_test.F90", "nemo_test.F90_TEST_PR"], cwd=self.base_dir)
        self.execute("diff nemo_test.F90 nemo_test.F90_TEST_PR")
        
        # Cleanup
        self.execute("make clean")
        
        self.execute("diff nemo_test.F90 nemo_test.F90_TEST_ORIG")

if __name__ == '__main__':
    unittest.main()
