#include <stdio.h>
#include <stdlib.h>
#include <papi.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define MAX_EVENTS 256
// Size of one CAS (cache line) in bytes; typically 64 bytes.
#define BYTES_PER_CAS 64.0

int main(int argc, char *argv[])
{
    int retval;
    int EventSet = PAPI_NULL;
    long long values[MAX_EVENTS];
    long long values2[MAX_EVENTS];
    char *events[MAX_EVENTS];
    int num_events = 0;
    double interval = 1.0;

    // Get events from environment variable
    const char *env_counters = getenv("PERF_REGIONS_COUNTERS");
    if (env_counters != NULL) {
        char *counters_copy = strdup(env_counters);
        char *token = strtok(counters_copy, ",");
        while (token != NULL) {
            if (num_events >= MAX_EVENTS) {
                fprintf(stderr, "Error: Too many events (max %d)\n", MAX_EVENTS);
                free(counters_copy);
                return 1;
            }
            events[num_events++] = strdup(token);
            token = strtok(NULL, ",");
        }
        free(counters_copy);
    }

    // Parse command line arguments
    int opt;
    double timeout = 0.0;

    // Extend argument parsing to handle -t (timeout)
    // We consume all arguments here so the original loop below skips execution
    while ((opt = getopt(argc, argv, "i:e:t:")) != -1) {
        switch (opt) {
        case 'i':
            interval = atof(optarg);
            if (interval <= 0.0) {
                 fprintf(stderr, "Warning: Invalid interval %.2f, defaulting to 1.0s\n", interval);
                 interval = 1.0;
            }
            break;
        case 'e':
            if (num_events >= MAX_EVENTS) {
                fprintf(stderr, "Error: Too many events (max %d)\n", MAX_EVENTS);
                return 1;
            }
            events[num_events++] = strdup(optarg);
            break;
        case 't':
            timeout = atof(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s [-i interval] [-e event] [-t timeout]...\n", argv[0]);
            return 1;
        }
    }

    struct timespec start_tp;
    if (timeout > 0.0) {
        clock_gettime(CLOCK_MONOTONIC, &start_tp);
    }
    while ((opt = getopt(argc, argv, "i:e:")) != -1) {
        switch (opt) {
        case 'i':
            interval = atof(optarg);
            if (interval <= 0.0) {
                 fprintf(stderr, "Warning: Invalid interval %.2f, defaulting to 1.0s\n", interval);
                 interval = 1.0;
            }
            break;
        case 'e':
            if (num_events >= MAX_EVENTS) {
                fprintf(stderr, "Error: Too many events (max %d)\n", MAX_EVENTS);
                return 1;
            }
            events[num_events++] = strdup(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s [-i interval] [-e event]...\n", argv[0]);
            return 1;
        }
    }

    if (num_events == 0) {
        fprintf(stderr, "Error: No events found. Set PERF_REGIONS_COUNTERS or use -e.\n");
        return 1;
    }

    // Determine if we can run in IMC bandwidth mode
    // We expect pairs of RD, WR events
    int imc_mode = 0;
    int num_imc = 0;
    if (num_events % 2 == 0) {
        imc_mode = 1;
        num_imc = num_events / 2;
    }

    // Initialize PAPI
    retval = PAPI_library_init(PAPI_VER_CURRENT);
    if (retval != PAPI_VER_CURRENT && retval > 0)
    {
        fprintf(stderr, "PAPI_library_init failed: %d\n", retval);
        return 1;
    }

    retval = PAPI_create_eventset(&EventSet);
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI_create_eventset failed: %d\n", retval);
        return 1;
    }

    // Add events
    for (int i = 0; i < num_events; i++) {
        int code;
        retval = PAPI_event_name_to_code(events[i], &code);
        if (retval != PAPI_OK) {
            fprintf(stderr, "Failed to translate %s: %s\n",
                    events[i], PAPI_strerror(retval));
            return 1;
        }
        retval = PAPI_add_event(EventSet, code);
        if (retval != PAPI_OK) {
            fprintf(stderr, "Failed to add event %s: %s\n",
                    events[i], PAPI_strerror(retval));
            return 1;
        }
    }

    // Start counters
    retval = PAPI_start(EventSet);
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI_start failed: %s\n", PAPI_strerror(retval));
        return 1;
    }

    printf("PAPI Monitor (Press Ctrl+C to stop)\n");
    if (imc_mode) {
        printf("IMC Bandwidth Mode detected (%d IMCs)\n", num_imc);
        printf("IMC\tRD\tWR\tBW(GB/s)\n");
    } else {
        printf("Raw Event Mode\n");
    }
    printf("--------------------------------\n");

    while (1) {
        // First snapshot
        retval = PAPI_read(EventSet, values);
        if (retval != PAPI_OK) {
            fprintf(stderr, "PAPI_read (1) failed: %s\n", PAPI_strerror(retval));
            break;
        }

        // Sleep for measurement interval
        struct timespec req;
        req.tv_sec = (time_t)interval;
        req.tv_nsec = (long)((interval - req.tv_sec) * 1e9);
        nanosleep(&req, NULL);

        // Second snapshot
        retval = PAPI_read(EventSet, values2);
        if (retval != PAPI_OK) {
            fprintf(stderr, "PAPI_read (2) failed: %s\n", PAPI_strerror(retval));
            break;
        }

        // Clear screen and print header
        printf("\033[2J\033[H");  // ANSI clear screen + move cursor to top-left

        printf("Monitor (%.1fs interval, Press Ctrl+C)\n", interval);
        if (imc_mode) {
            printf("IMC\tRD/s\tWR/s\tBW(GB/s)\n");
            printf("--------------------------------\n");

            double total_bandwidth = 0.0;
            // Compute and print bandwidth per IMC
            for (int i = 0; i < num_imc; i++) {
                long long rd = values2[i*2 + 0] - values[i*2 + 0];
                long long wr = values2[i*2 + 1] - values[i*2 + 1];

                double bytes = (rd + wr) * BYTES_PER_CAS;
                double bandwidth_GBps = bytes / interval / 1e9;

                printf("IMC%d\t%lld\t%lld\t%.2f\n",
                       i, rd, wr, bandwidth_GBps);
                total_bandwidth += bandwidth_GBps;
            }
            printf("--------------------------------\n");
            printf("Total Bandwidth: %.2f GB/s\n", total_bandwidth);
        } else {
            printf("Event\tCount\tRate/s\n");
            printf("--------------------------------\n");
            for(int i=0; i<num_events; i++) {
                 long long count = values2[i] - values[i];
                 double rate = count / interval;
                 printf("%s\t%lld\t%.2e\n", events[i], count, rate);
            }
        }

        if (timeout > 0.0) {
            struct timespec current_tp;
            clock_gettime(CLOCK_MONOTONIC, &current_tp);
            double elapsed = (double)(current_tp.tv_sec - start_tp.tv_sec) + 
                             (double)(current_tp.tv_nsec - start_tp.tv_nsec) / 1e9;
            if (elapsed > timeout) {
                break;
            }
        }
    }

    PAPI_stop(EventSet, NULL);
    PAPI_shutdown();
    
    // Cleanup
    for(int i=0; i<num_events; i++) {
        free(events[i]);
    }
    
    return 0;
}


