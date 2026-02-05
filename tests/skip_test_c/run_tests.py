import unittest
import subprocess
import os
import json

class TestSkipTest(unittest.TestCase):
    def setUp(self):
        # Current file is in tests/skip_test_c/
        self.base_dir = os.path.dirname(os.path.abspath(__file__))
        self.env = os.environ.copy()
        
        # Library path setup - pointing to src/ folder where libperf_regions.so lives
        src_path = os.path.abspath(os.path.join(self.base_dir, "../../src"))
        existing_ld = self.env.get("LD_LIBRARY_PATH", "")
        self.env["LD_LIBRARY_PATH"] = f"{src_path}:{existing_ld}"

        # Clean
        self.execute("make clean")
        
        # Default perf regions counters for this test (wallclock is sufficient for logic test)
        if "PERF_REGIONS_COUNTERS" not in self.env:
            self.env["PERF_REGIONS_COUNTERS"] = "WALLCLOCKTIME"

    def execute(self, command):
        subprocess.check_call(command.split(), cwd=self.base_dir, env=self.env)

    def test_skip_execution(self):
        """Test with SKIP_N=2. Expecting 3 updates and 2 skips for Outer region."""
        # Build
        self.execute("make skip_test")
        
        # Run with SKIP_N=2
        run_env = self.env.copy()
        run_env["PERF_REGIONS_SKIP_N"] = "2"
        run_env["PERF_REGIONS_OUTPUT"] = "json=perf_regions_output.json"
        
        # Remove old output
        json_path = os.path.join(self.base_dir, "perf_regions_output.json")
        if os.path.exists(json_path):
            os.remove(json_path)

        # Run
        subprocess.check_call(["./skip_test"], cwd=self.base_dir, env=run_env)
        
        # Verify
        self.assertTrue(os.path.exists(json_path), "JSON output file not created")
        
        with open(json_path, 'r') as f:
            data = json.load(f)
            
        # Parse data. Assuming list of objects.
        outer = next((r for r in data if r['region'] == 'Outer'), None)
        independent = next((r for r in data if r['region'] == 'Independent'), None)
        inner = next((r for r in data if r['region'] == 'Inner'), None)
        
        self.assertIsNotNone(outer, "Outer region not found in JSON")
        self.assertIsNotNone(independent, "Independent region not found in JSON")
        self.assertIsNotNone(inner, "Inner region not found in JSON")
        
        # Validation Logic:
        # Loop 5 times. Skip 2. Count should be 3. Skipped should be 2.
        
        # Outer
        self.assertEqual(outer.get('counter'), 3, f"Outer region count should be 3, got {outer.get('counter')}")
        self.assertEqual(outer.get('skipped'), 2, f"Outer region skipped should be 2, got {outer.get('skipped')}")
        
        # Independent (also loop 5)
        self.assertEqual(independent.get('counter'), 3,
                         f"Independent count should be 3, got {independent.get('counter')}")
        self.assertEqual(independent.get('skipped'), 2,
                         f"Independent skipped should be 2, got {independent.get('skipped')}")

    def test_no_skip_execution(self):
        """Test with SKIP_N=0. Expecting 5 updates and 0 skips."""
        # Build
        self.execute("make skip_test")
        
        # Run with SKIP_N=0
        run_env = self.env.copy()
        run_env["PERF_REGIONS_SKIP_N"] = "0"
        run_env["PERF_REGIONS_OUTPUT"] = "json=perf_regions_output_full.json"
        
        json_path = os.path.join(self.base_dir, "perf_regions_output_full.json")
        if os.path.exists(json_path):
            os.remove(json_path)

        subprocess.check_call(["./skip_test"], cwd=self.base_dir, env=run_env)
        
        with open(json_path, 'r') as f:
            data = json.load(f)
            
        outer = next((r for r in data if r['region'] == 'Outer'), None)
        self.assertEqual(outer.get('counter'), 5, f"Outer region count should be 5 with SKIP_N=0, got {outer.get('counter')}")
        self.assertEqual(outer.get('skipped'), 0, f"Outer region skipped should be 0, got {outer.get('skipped')}")

if __name__ == '__main__':
    unittest.main()
