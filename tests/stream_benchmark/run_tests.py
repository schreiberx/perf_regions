import unittest
import subprocess
import os
import sys

class TestStreamBenchmark(unittest.TestCase):
    def setUp(self):
        self.base_dir = os.path.dirname(os.path.abspath(__file__))
        self.env = os.environ.copy()
        
        # Library path setup
        src_path = os.path.abspath(os.path.join(self.base_dir, "../../src"))
        existing_ld = self.env.get("LD_LIBRARY_PATH", "")
        self.env["LD_LIBRARY_PATH"] = f"{src_path}:{existing_ld}"

        # Counters
        self.events_cacheblocks = "PAPI_L3_TCM".split(",")
        perf_regions_counters = ",".join(self.events_cacheblocks) + ",WALLCLOCKTIME"
        if "PERF_REGIONS_COUNTERS" not in self.env:
            self.env["PERF_REGIONS_COUNTERS"] = perf_regions_counters
        
        # Clean
        subprocess.check_call(["make", "clean"], cwd=self.base_dir, env=self.env)

    def test_stream_execution(self):
        # Build
        # make stream_c_perfregions
        print(f"Building in {self.base_dir}")
        subprocess.check_call(["make", "stream_c_perfregions"], cwd=self.base_dir, env=self.env)
        
        # Run
        print("Running ./stream_c_perfregions")
        try:
            output = subprocess.check_output(["./stream_c_perfregions"], cwd=self.base_dir, env=self.env, stderr=subprocess.STDOUT, text=True)
        except subprocess.CalledProcessError as e:
            print(e.output)
            self.fail("stream_c_perfregions execution failed")
            
        if "ERROR" in output:
            print(output)
            self.fail("Detected ERROR in output")
            
        # Parse Output (Simplified version of original logic)
        print(output)
        
        pr_tag = "[MULE] perf_regions: "
        data_table = []
        for line in output.splitlines():
            if not line.startswith(pr_tag):
                continue
            cols = line[len(pr_tag):].split()
            data_table.append(cols)

        if not data_table:
             print("No perf_regions output found - maybe PAPI not available?")
             return

        headers = data_table[0]
        rows = data_table[1:]
        
        # Determine Checksum of Logic (just print simple analysis like before, but without pandas)
        # Find indices
        try:
            l3_idx = headers.index("PAPI_L3_TCM")
            wall_idx = headers.index("WALLCLOCKTIME")
        except ValueError:
             # Maybe PAPI not available or different configuration
             print(f"Headers found: {headers}. Missing expected columns for bandwidth calc.")
             print("Skipping bandwidth check.")
             return

        cacheblock_size = 64
        print("\nManual Bandwidth Calculation:")
        print(f"{'Region':<15} {'Bandwidth (MB/s)':<20}")
        
        for row in rows:
            region = row[0] 
            
            try:
                misses = float(row[l3_idx])
                wall_time = float(row[wall_idx])
                
                if wall_time > 0:
                     bw = misses * cacheblock_size / (wall_time * 1000.0 * 1000.0)
                     print(f"{region:<15} {bw:<20.4f}")
                else:
                     print(f"{region:<15} {'N/A (Time=0)':<20}")
            except (ValueError, IndexError):
                pass
        
        print("=" * 80)
        print("| WARNING: TCM counters may not be accurate on some systems. |")

if __name__ == '__main__':
    unittest.main()
