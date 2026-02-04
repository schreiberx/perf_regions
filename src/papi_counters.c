#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if PERF_REGIONS_USE_PAPI
#include <papi.h>
#include "papi_counters.h"
#endif


#define PRINT_PREFIX "[papi_counters.c] "


#if PERF_REGIONS_USE_PAPI

/**
 * Struct to gather all variables
 */
struct PapiCounters {

	/*
	 * Verbosity level (0=no verbosity)
	 */
	int verbosity;

	/*
	 * PAPI event set
	 */
	int event_set;

	/*
	 * Number of used performance counters
	 */
	long long int event_list_len;

	/*
	 * Array of strings for performance counters
	 */
	char *const * event_list;
};

static struct PapiCounters papi_counters;


/**
 * Some hopefully useful information if someone encounters problems.
 */
void print_access_right_problems()
{
	fprintf(stderr, "\
Error occurred. Maybe that's a problem of missing access rights?\n\
\n\
Note, that the following recommendations can lead to SECURITY issues on multi-user systems!\n\
\n\
Activate perf counters for all users:\n\
$ sudo bash -c 'echo \"-1\" > /proc/sys/kernel/perf_event_paranoid'\n\
\n\
You can also set this permanently by adding the line\n\
    kernel.perf_event_paranoid = -1\n\
to\n\
    /etc/sysctl.conf\n\
\n\
");
	assert(0);
}



void handle_error(const char *i_error_msg, int retval)
{
	fprintf(stderr, PRINT_PREFIX"%s", i_error_msg);
	fprintf(stderr, ": retval=%i", retval);
	exit(EXIT_FAILURE);
}



/**
 * Initialize performance counters *
 */
void papi_counters_init(
	char *const list_counters[],
	int i_num_counters,
	int i_verbosity
)
{
	papi_counters.verbosity = i_verbosity;
	papi_counters.event_list_len = i_num_counters;
	papi_counters.event_list = list_counters;

	int retval;

	retval = PAPI_library_init(PAPI_VER_CURRENT);
	if (retval != PAPI_VER_CURRENT && retval > 0)
		handle_error(PRINT_PREFIX"library version mismatch", retval);

	if (retval < 0)
		handle_error("PAPI library init error", retval);

	retval = PAPI_is_initialized();
	if (retval == PAPI_NOT_INITED)
		handle_error("PAPI library not initialized", retval);

	papi_counters.event_set = PAPI_NULL;
	retval = PAPI_create_eventset(&papi_counters.event_set);
	if (retval != PAPI_OK)
		handle_error("PAPI: Creation of eventset failed", retval);

	for (int i = 0; i < papi_counters.event_list_len; i++)
	{
		if (papi_counters.verbosity > 0)
			printf(PRINT_PREFIX"Adding event '%s'\n", papi_counters.event_list[i]);
			
		if ((retval = PAPI_add_named_event(papi_counters.event_set, papi_counters.event_list[i])) != PAPI_OK)
		{
			fprintf(stderr, PRINT_PREFIX"PAPI_add_named_event failed for '%s'! retval: %d\n", papi_counters.event_list[i], retval);
			print_access_right_problems();
			exit(-1);
		}
	}
}



/**
 * Shutdown performance counters
 * 
 * We undo everything inclulding shutting down the PAPI library
 */
void papi_counters_finalize()
{
	int retval;
	for (int i = 0; i < papi_counters.event_list_len; i++)
	{
		if (papi_counters.verbosity > 0)
			printf(PRINT_PREFIX"Removing event '%s'\n", papi_counters.event_list[i]);

		if ((retval = PAPI_remove_named_event(papi_counters.event_set, papi_counters.event_list[i])) != PAPI_OK)
		{
			fprintf(stderr, PRINT_PREFIX"PAPI_remove_named_event failed for '%s'! retval: %d\n", papi_counters.event_list[i], retval);
			print_access_right_problems();
			exit(-1);
		}
	}

	retval = PAPI_destroy_eventset(&papi_counters.event_set);
	if (retval != PAPI_OK)
		handle_error(PRINT_PREFIX"Creation of eventset failed", retval);

	papi_counters.event_set = 0;

	PAPI_shutdown();
}


/**
 * start performance counting. Existing performance counters are overwritten!
 */
void papi_counters_start()
{
	// start the event, use default (1st) branch as return branch
	if (PAPI_start(papi_counters.event_set) == PAPI_OK)
		return;

	// suppress errors if no perf counters are activated
	if (papi_counters.event_list_len != 0)
	{
		PAPI_perror("PAPI_start_counters");
		print_access_right_problems();
		exit(-1);
	}
}


/**
 * stop performance counters and accumulate everything to array values
 */
void papi_counters_stop_and_accum(
	long long *o_count_values_accum
) {
	// stop event and accumulate counters
	if (PAPI_accum(papi_counters.event_set, o_count_values_accum) == PAPI_OK)
		return;

	if (papi_counters.event_list_len != 0)
	{
		PAPI_perror("PAPI_accum_counters: ");
		print_access_right_problems();
		exit(-1);
	}
}


/**
 * stop performance counters and store everything to array values
 */
void papi_counters_stop(
	long long *o_count_values_stop
) {
	// stop event and accumulate counters
	if (PAPI_stop(papi_counters.event_set, o_count_values_stop) == PAPI_OK)
		return;

	if (papi_counters.event_list_len != 0)
	{
		PAPI_perror("PAPI_stop_counters: ");
		print_access_right_problems();
		exit(-1);
	}
}


/**
 * Read the counters into count_values_read
 */
void papi_counters_read_and_reset(
		long long *o_count_values_read
) {
	// read the counters
	if (PAPI_read(papi_counters.event_set, o_count_values_read) == PAPI_OK)
	{
		if (PAPI_reset(papi_counters.event_set) == PAPI_OK)
		{
			return;
		}
		else
		{
			PAPI_perror("PAPI_reset_counters: ");
			print_access_right_problems();
			exit(-1);
		}
	}

	if (papi_counters.event_list_len != 0)
	{
		PAPI_perror("PAPI_read_counters: ");
		print_access_right_problems();
		exit(-1);
	}
}

#endif
