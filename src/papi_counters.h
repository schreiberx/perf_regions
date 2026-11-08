#ifndef PAPI_COUNTERS_H
#define PAPI_COUNTERS_H

#include "perf_regions_defines.h"

#if PERF_REGIONS_USE_PAPI

void papi_counters_library_init(void);
void papi_counters_init(char *const list_counters[], int i_num_counters, int i_verbosity);
void papi_counters_finalize(void);

void papi_counters_start(void);
void papi_counters_stop(long long *o_count_values_stop);

void papi_counters_stop_and_accum(long long *io_count_values_accum);
void papi_counters_read_and_reset(long long *o_count_values_read);

#endif

#endif
