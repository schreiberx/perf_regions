#include <stdio.h>
#include <unistd.h>
#include "perf_regions.h"

void compute_something() {
    // Simulate some computation
    for (int i = 0; i < 1000000; i++) {
        volatile int x = i * i;
    }
}

void io_operation() {
    // Simulate I/O
    usleep(10000);  // Sleep for 10ms
}

int main() {
    printf("Example usage of perf_regions library\n");
    
    // Initialize the performance measurement system
    perf_regions_init();
    
    // Measure computation region
    perf_region_start(1, "computation");
    compute_something();
    perf_region_stop(1);
    
    // Measure I/O region
    perf_region_start(2, "io_operations");
    io_operation();
    io_operation();
    perf_region_stop(2);
    
    // Measure nested regions
    perf_region_start(3, "outer_region");
    
        perf_region_start(4, "inner_region");
        compute_something();
        perf_region_stop(4);
        
        io_operation();
    
    perf_region_stop(3);
    
    // Output results in human-readable format
    printf("\nPerformance Results:\n");
    printf("====================\n");
    perf_regions_output_human_readable_text();
    
    // Also output CSV format
    printf("\nCSV Output:\n");
    printf("===========\n");
    perf_regions_output_csv();
    
    // Clean up
    perf_regions_finalize();
    
    return 0;
}