#include "papi_counters.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <papi.h>


static long long *count_values;
static char *count_event_list_string[PERF_COUNTERS_MAX];
static char *count_event_string_buffer;
static int *count_event_code;
static int count_num_counters;
static char *count_list_counters;



void print_access_right_problems()
{
	fprintf(
			stderr, "(from perftools)\n\
\n\
	Consider tweaking /proc/sys/kernel/perf_event_paranoid,\n\
	which controls use of the performance events system by\n\
	unprivileged users (without CAP_SYS_ADMIN).\n\
\n\
	The current value is 3:\n\
\n\
	  -1: Allow use of (almost) all events by all users\n\
	>= 0: Disallow raw tracepoint access by users without CAP_IOC_LOCK\n\
	>= 1: Disallow CPU event access by users without CAP_SYS_ADMIN\n\
	>= 2: Disallow kernel profiling by users without CAP_SYS_ADMIN\n\
");
}
/**
 * Initialize performance counters based on environment variables
 */
int count_init()
{
	if (getenv("LIST_COUNTERS") == NULL)
	{
		printf("LIST_COUNTERS is not defined - dummy counting activated!\n");
		count_list_counters = "";
	}
	else
	{
		count_list_counters = getenv("LIST_COUNTERS");
	}

	int retval;
	if (PAPI_is_initialized() == PAPI_NOT_INITED)
	{
		if ((retval = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT)
		{
			fprintf(stderr, "PAPI library init error! retval: %d\n", retval);
			return 0;
		}
	}

	count_event_string_buffer = strdup(count_list_counters);

	char *events = strtok(count_event_string_buffer, ",");

	int i = 0;
	while (events != NULL)
	{
		count_event_list_string[i] = events;
		events = strtok(NULL, ",");
		i++;
	}

	count_num_counters = i;
//	printf("NUM: %i\n", count_num_counters);

	if (count_num_counters > PERF_COUNTERS_MAX)
	{
		printf("Maximum number of performance counters (%i) reached\n", PERF_COUNTERS_MAX);
		exit(1);
	}

	count_values = malloc(count_num_counters*sizeof(long long));
	count_event_code = malloc(count_num_counters*sizeof(int));

//	fprintf(stderr, "Setting up event codes");
	for (int i = 0; i < count_num_counters; i++)
	{
		if ((retval = PAPI_event_name_to_code(count_event_list_string[i], &count_event_code[i])) != PAPI_OK)
		{
			fprintf(stderr, "PAPI_event_name_to_code failed! retval: %d\n", retval);
			print_access_right_problems();
			exit(-1);
		}

		count_values[i] = 0;
	}

//	for (int i = 0; i < count_num_counters; i++)
//		fprintf(stderr, "%i\n", count_event_code[i]);
	return 0;
}



/**
 * start performance counting. Existing performance counters are overwritten! (TODO: Check this)
 */
int count_start() {
	// start the event, use default (1st) branch ad return branch
	if (PAPI_start_counters(count_event_code, count_num_counters) == PAPI_OK)
		return 0;

	// suppress errors if no perf counters are activated
	if (count_num_counters != 0)
	{
		PAPI_perror("PAPI_start: ");
		print_access_right_problems();
		exit(-1);
	}

	return 0;

}



/** 
 * stop performance counters and store everything to array values
 */
int count_stop() {
	// stop the event
	if (PAPI_stop_counters(count_values, count_num_counters) == PAPI_OK)
		return 0;

	if (count_num_counters != 0)
	{
		PAPI_perror("PAPI_stop: ");
		print_access_right_problems();
		exit(-1);
	}

	return 0;
}

/**
 * shutdown performance counters
 */
int count_finalize()
{
	free(count_values);
	free(count_event_code);
	free(count_event_string_buffer);

	return 0;
}


/**
 * return pointer to array of counter values
 */
long long *count_get_valueptr()
{
	return count_values;
}



/**
 * return pointer to array of event names
 */
char **count_get_event_names()
{
	return count_event_list_string;
}

/**
 * return number of performance counters
 */
int count_get_num()
{
	return count_num_counters;
}
