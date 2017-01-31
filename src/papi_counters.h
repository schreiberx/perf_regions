
// "Maximum number of performance counters which may be used. Feel free to increase this number"
#define PERF_COUNTERS_MAX		(256)


void count_init();

void count_start();


void count_accum(long long *io_count_values_accum);

void count_stop(long long *o_count_values_stop);

void count_read_and_reset(long long *o_count_values_read);


void count_finalize();

char **count_get_event_names();

int count_get_num();

