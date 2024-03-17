
// "Maximum number of performance counters which may be used. Feel free to increase this number"
#define PERF_COUNTERS_MAX		(256)


void papi_counters_init(char *const list_counters[], int i_num_counters, int i_verbosity);
void papi_counters_finalize();


void papi_counters_start();
void papi_counters_stop(long long *o_count_values_stop);

void papi_counters_stop_and_accum(long long *io_count_values_accum);
void papi_counters_read_and_reset(long long *o_count_values_read);

