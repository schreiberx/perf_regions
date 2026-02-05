#define _DEFAULT_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <perf_regions.h>

int main() {
    perf_regions_init();
    
    printf("Running skip test...\n");

    // Run 5 times.
    // If PERF_REGIONS_SKIP_N=2:
    // Iteration 0: Outer skipped. Inner skipped (nested).
    // Iteration 1: Outer skipped. Inner skipped (nested).
    // Iteration 2: Outer active. Inner active (count 0 check? wait)

    // Wait, inner region counter logic:
    // Inner region is started 5 times.
    // Inner region count goes 0, 1, 2, 3, 4.
    
    // If Outer is skipped (0, 1):
    // Inner is skipped due to parent.
    
    // Iteration 2 (Outer count 2):
    // Outer Active.
    // Inner Start. Inner count 2.
    // Check inner count < SKIP_N (2) ? No. 2 < 2 is False.
    // So Inner Active.
    
    // So Inner runs 3 times (indices 2, 3, 4).
    // Outer runs 3 times (indices 2, 3, 4).
    
    for(int i=0; i<5; i++) {
        perf_region_start(0, "Outer");
            usleep(1000); // 1ms
            
            perf_region_start(1, "Inner");
                usleep(1000); // 1ms
            perf_region_stop(1);

        perf_region_stop(0);
    }
    
    // Test independent region
    // Region 2: check if it skips 2 times correctly
    for(int i=0; i<5; i++) {
         perf_region_start(2, "Independent");
         usleep(1000);
         perf_region_stop(2);
    }

    perf_regions_finalize();
    return 0;
}
