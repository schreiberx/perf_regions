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

        subprocess.check_call(["make", "clean"], cwd=self.base_dir, env=self.env)

    def test_variable_sizes(self):
        # Build
        print("Building nemo_test_perf_regions")
        subprocess.check_call(["make"], cwd=self.base_dir, env=self.env)
        
        sizes = [1, 2, 4, 8, 16, 32, 64, 128]
        for i in sizes:
            print(f"\n***********************************")
            print(f"SIZE: {i}")
            print(f"***********************************")
            
            run_env = self.env.copy()
            run_env["JPI"] = str(i)
            run_env["JPJ"] = str(i)
            run_env["JPK"] = "72"
            
            # The shell script re-exports PERF_REGIONS_COUNTERS inside the loop, 
            # but it seems to just overwrite it with the same value or a fixed one.
            # Original: export PERF_REGIONS_COUNTERS=FP_COMP_OPS_EXE,PAPI_L3_TCM
            #           export PERF_REGIONS_COUNTERS=PAPI_L3_TCM,WALLCLOCKTIME
            # The second one wins.
            run_env["PERF_REGIONS_COUNTERS"] = "PAPI_L3_TCM,WALLCLOCKTIME"
            
            subprocess.check_call(["taskset", "-c", "0", "./nemo_test_perf_regions"], cwd=self.base_dir, env=run_env)

if __name__ == '__main__':
    unittest.main()
