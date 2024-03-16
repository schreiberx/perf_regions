#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <papi.h>

#include "papi_counters.h"


/**
 * Struct to gather all variables
 */
struct PapiCounters {

	/*
	 * PAPI event set
	 */
	int event_set;

	/*
	 * Number of used performance counters
	 */
	long long int event_list_len;

	/*
	 * String which is used to break down the comma-separated list.
	 * This is referenced to by the count_event_list_string[]
	 */
	char *event_string_buffer;

	/*
	 * Array of strings for performance counters
	 */
	const char * const*event_list;
};

static struct PapiCounters papi_counters;


/**
 * Some hopefully useful information if someone encounters problems.
 */
void print_access_right_problems()
{
	fprintf(
			stderr, "\
Error occurred, maybe that's a problem of missing access rights?\n\
\n\
(from perftools)\n\
\n\
	Consider tweaking /proc/sys/kernel/perf_event_paranoid,\n\
	which controls use of the performance events system by\n\
	unprivileged users (without CAP_SYS_ADMIN).\n\
\n\
	  -1: Allow use of (almost) all events by all users\n\
	>= 0: Disallow raw tracepoint access by users without CAP_IOC_LOCK\n\
	>= 1: Disallow CPU event access by users without CAP_SYS_ADMIN\n\
	>= 2: Disallow kernel profiling by users without CAP_SYS_ADMIN\n\
");
	assert(0);
}



void handle_error(const char *i_error_msg, int retval)
{
	fprintf(stderr, "%s", i_error_msg);
	fprintf(stderr, ": retval=%i", retval);
	exit(EXIT_FAILURE);
}



/**
 * Initialize performance counters
 *
 * The initialization is based on the environment variable LIST_COUNTERS
 */
void papi_counters_init(const char * const* list_counters, int i_num_counters)
{
	papi_counters.event_list_len = i_num_counters;
	papi_counters.event_list = list_counters;

	int retval;

	retval = PAPI_library_init(PAPI_VER_CURRENT);
	if (retval != PAPI_VER_CURRENT && retval > 0)
		handle_error("PAPI library version mismatch", retval);

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
		fprintf(stderr, "Setting up event code for '%s'\n", papi_counters.event_list[i]);
		if ((retval = PAPI_add_named_event(papi_counters.event_set, papi_counters.event_list[i])) != PAPI_OK)
		{
			fprintf(stderr, "PAPI_event_name_to_code failed! retval: %d\n", retval);
			print_access_right_problems();
			exit(-1);
		}
	}
}


/**
 * start performance counting. Existing performance counters are overwritten! (TODO: Check this)
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
		return;

	if (papi_counters.event_list_len != 0)
	{
		PAPI_perror("PAPI_read_counters: ");
		print_access_right_problems();
		exit(-1);
	}
}


/**
 * shutdown performance counters
 */
void papi_counters_finalize()
{
	free(papi_counters.event_string_buffer);
}
