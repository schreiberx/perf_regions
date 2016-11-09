
// "Maximum number of performance counters which may be used. Feel free to increase this number"
#define PERF_COUNTERS_MAX		(256)


int count_init();

int count_start();

int count_stop();

int count_finalize();

long long *count_get_valueptr();

char **count_get_event_names();

int count_get_num();

