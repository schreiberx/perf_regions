
// "Maximum number of performance counters which may be used. Feel free to increase this number"
#define PERF_COUNTERS_MAX		(256)


void papi_counters_init();

void papi_counters_start();


void papi_counters_stop_and_accum(long long *io_count_values_accum);

void papi_counters_stop(long long *o_count_values_stop);

void papi_counters_read_and_reset(long long *o_count_values_read);


void papi_counters_finalize();

char **papi_counters_get_event_names();

int papi_counters_get_num_active_counters();

