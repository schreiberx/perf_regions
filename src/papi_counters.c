#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if PERF_REGIONS_USE_PAPI
#include <papi.h>
#include "papi_counters.h"
#endif


#define PRINT_PREFIX "[papi_counters.c] "


#if PERF_REGIONS_USE_PAPI

struct PapiEventSet
{
	/*
	 * PAPI component this event set belongs to.
	 */
	int component_index;

	/*
	 * PAPI event set.
	 */
	int event_set;

	/*
	 * Number of events in this set.
	 */
	int num_events;

	/*
	 * Indices into the global event list for events in this set.
	 */
	int *event_indices;
};

/**
 * Struct to gather all variables
 */
struct PapiCounters {

	/*
	 * Verbosity level (0=no verbosity)
	 */
	int verbosity;

	/*
	 * Number of used performance counters
	 */
	long long int event_list_len;

	/*
	 * Array of strings for performance counters
	 */
	char *const * event_list;

	/*
	 * PAPI has separate components for core and uncore events.  Keep one
	 * event set per component so both can be used in the same run.
	 */
	struct PapiEventSet *event_sets;
	int num_event_sets;

	/*
	 * Mapping from the global event list to the event set/local index.
	 */
	int *event_set_index;
	int *event_local_index;
};

static struct PapiCounters papi_counters;
static int papi_library_initialized = 0;
static int papi_counters_initialized = 0;


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
}



void handle_error(const char *i_error_msg, int retval)
{
	fprintf(stderr, PRINT_PREFIX"%s: retval=%i (%s)\n",
		i_error_msg, retval, PAPI_strerror(retval));
	exit(EXIT_FAILURE);
}


/**
 * Initialize the PAPI library only.  Event discovery needs this before the
 * final counter list (including expanded pseudo events) is known.
 */
void papi_counters_library_init(void)
{
	if (papi_library_initialized)
		return;

	int retval = PAPI_library_init(PAPI_VER_CURRENT);
	if (retval != PAPI_VER_CURRENT && retval > 0)
		handle_error(PRINT_PREFIX"library version mismatch", retval);

	if (retval < 0)
		handle_error("PAPI library init error", retval);

	retval = PAPI_is_initialized();
	if (retval == PAPI_NOT_INITED)
		handle_error("PAPI library not initialized", retval);

	papi_library_initialized = 1;
}


static void free_event_sets(void)
{
	if (papi_counters.event_sets != NULL)
	{
		for (int i = 0; i < papi_counters.num_event_sets; i++)
			free(papi_counters.event_sets[i].event_indices);
		free(papi_counters.event_sets);
	}

	free(papi_counters.event_set_index);
	free(papi_counters.event_local_index);

	papi_counters.event_sets = NULL;
	papi_counters.event_set_index = NULL;
	papi_counters.event_local_index = NULL;
	papi_counters.num_event_sets = 0;
}


static struct PapiEventSet *find_or_create_event_set(int component_index)
{
	for (int i = 0; i < papi_counters.num_event_sets; i++)
	{
		if (papi_counters.event_sets[i].component_index == component_index)
			return &papi_counters.event_sets[i];
	}

	struct PapiEventSet *set = &papi_counters.event_sets[papi_counters.num_event_sets];
	set->component_index = component_index;
	set->event_set = PAPI_NULL;
	set->num_events = 0;
	set->event_indices = (int *)calloc(
		(size_t)papi_counters.event_list_len, sizeof(int));
	if (set->event_indices == NULL)
	{
		fprintf(stderr, PRINT_PREFIX"Out of memory\n");
		exit(EXIT_FAILURE);
	}

	int retval = PAPI_create_eventset(&set->event_set);
	if (retval != PAPI_OK)
		handle_error("PAPI: Creation of eventset failed", retval);

	retval = PAPI_assign_eventset_component(set->event_set, component_index);
	if (retval != PAPI_OK)
		handle_error("PAPI: Assigning eventset component failed", retval);

	papi_counters.num_event_sets++;
	return set;
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

	if (i_num_counters <= 0)
		return;

	papi_counters_library_init();

	papi_counters.event_sets = (struct PapiEventSet *)calloc(
		(size_t)i_num_counters, sizeof(struct PapiEventSet));
	papi_counters.event_set_index = (int *)calloc(
		(size_t)i_num_counters, sizeof(int));
	papi_counters.event_local_index = (int *)calloc(
		(size_t)i_num_counters, sizeof(int));
	if (papi_counters.event_sets == NULL ||
	    papi_counters.event_set_index == NULL ||
	    papi_counters.event_local_index == NULL)
	{
		fprintf(stderr, PRINT_PREFIX"Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < i_num_counters; i++)
	{
		int event_code = 0;
		int retval = PAPI_event_name_to_code(papi_counters.event_list[i], &event_code);
		if (retval != PAPI_OK)
		{
			fprintf(stderr, PRINT_PREFIX"PAPI_event_name_to_code failed for '%s'! retval: %d\n",
				papi_counters.event_list[i], retval);
			print_access_right_problems();
			exit(EXIT_FAILURE);
		}

		int component_index = PAPI_get_event_component(event_code);
		if (component_index < 0)
		{
			fprintf(stderr, PRINT_PREFIX"Could not determine PAPI component for '%s'! retval: %d\n",
				papi_counters.event_list[i], component_index);
			exit(EXIT_FAILURE);
		}

		struct PapiEventSet *set = find_or_create_event_set(component_index);
		int local_index = set->num_events;
		int set_index = (int)(set - papi_counters.event_sets);

		papi_counters.event_set_index[i] = set_index;
		papi_counters.event_local_index[i] = local_index;
		set->event_indices[local_index] = i;
		set->num_events++;

		if (papi_counters.verbosity > 0)
			printf(PRINT_PREFIX"Adding event '%s' (component %d)\n",
				papi_counters.event_list[i], component_index);

		if ((retval = PAPI_add_named_event(set->event_set, papi_counters.event_list[i])) != PAPI_OK)
		{
			fprintf(stderr, PRINT_PREFIX"PAPI_add_named_event failed for '%s'! retval: %d\n",
				papi_counters.event_list[i], retval);
			print_access_right_problems();
			exit(EXIT_FAILURE);
		}
	}

	papi_counters_initialized = 1;
}



/**
 * Shutdown performance counters
 *
 * We undo everything including shutting down the PAPI library.
 */
void papi_counters_finalize(void)
{
	if (papi_counters_initialized)
	{
		int retval;
		for (int i = 0; i < papi_counters.num_event_sets; i++)
		{
			struct PapiEventSet *set = &papi_counters.event_sets[i];
			for (int j = 0; j < set->num_events; j++)
			{
				int global_index = set->event_indices[j];
				if (papi_counters.verbosity > 0)
					printf(PRINT_PREFIX"Removing event '%s'\n",
						papi_counters.event_list[global_index]);

				if ((retval = PAPI_remove_named_event(set->event_set,
						papi_counters.event_list[global_index])) != PAPI_OK)
				{
					fprintf(stderr, PRINT_PREFIX"PAPI_remove_named_event failed for '%s'! retval: %d\n",
						papi_counters.event_list[global_index], retval);
				}
			}

			retval = PAPI_destroy_eventset(&set->event_set);
			if (retval != PAPI_OK)
				fprintf(stderr, PRINT_PREFIX"PAPI_destroy_eventset failed: retval: %d\n", retval);
			set->event_set = PAPI_NULL;
		}

		papi_counters_initialized = 0;
	}

	free_event_sets();

	if (papi_library_initialized)
	{
		PAPI_shutdown();
		papi_library_initialized = 0;
	}
}


/**
 * start performance counting. Existing performance counters are overwritten!
 */
void papi_counters_start(void)
{
	if (!papi_counters_initialized)
		return;

	for (int i = 0; i < papi_counters.num_event_sets; i++)
	{
		struct PapiEventSet *set = &papi_counters.event_sets[i];
		if (PAPI_start(set->event_set) == PAPI_OK)
			continue;

		// suppress errors if no perf counters are activated
		if (papi_counters.event_list_len != 0)
		{
			PAPI_perror("PAPI_start_counters");
			print_access_right_problems();
			exit(EXIT_FAILURE);
		}
	}
}


/**
 * stop performance counters and accumulate everything to array values
 */
void papi_counters_stop_and_accum(
	long long *io_count_values_accum
) {
	if (!papi_counters_initialized)
		return;

	for (int i = 0; i < papi_counters.num_event_sets; i++)
	{
		struct PapiEventSet *set = &papi_counters.event_sets[i];
		long long local_values[PERF_COUNTERS_MAX];

		for (int j = 0; j < set->num_events; j++)
			local_values[j] = io_count_values_accum[set->event_indices[j]];

		if (PAPI_accum(set->event_set, local_values) != PAPI_OK)
		{
			if (papi_counters.event_list_len != 0)
			{
				PAPI_perror("PAPI_accum_counters: ");
				print_access_right_problems();
				exit(EXIT_FAILURE);
			}
			continue;
		}

		for (int j = 0; j < set->num_events; j++)
			io_count_values_accum[set->event_indices[j]] = local_values[j];
	}
}


static void add_local_values_to_global(
	long long *global_values,
	const struct PapiEventSet *set,
	const long long *local_values
)
{
	for (int j = 0; j < set->num_events; j++)
		global_values[set->event_indices[j]] = local_values[j];
}


/**
 * stop performance counters and store everything to array values
 */
void papi_counters_stop(
	long long *o_count_values_stop
) {
	if (!papi_counters_initialized)
		return;

	memset(o_count_values_stop, 0,
		(size_t)papi_counters.event_list_len * sizeof(long long));

	for (int i = 0; i < papi_counters.num_event_sets; i++)
	{
		struct PapiEventSet *set = &papi_counters.event_sets[i];
		long long local_values[PERF_COUNTERS_MAX];

		if (PAPI_stop(set->event_set, local_values) != PAPI_OK)
		{
			if (papi_counters.event_list_len != 0)
			{
				PAPI_perror("PAPI_stop_counters: ");
				print_access_right_problems();
				exit(EXIT_FAILURE);
			}
			continue;
		}

		add_local_values_to_global(o_count_values_stop, set, local_values);
	}
}


/**
 * Read the counters into count_values_read
 */
void papi_counters_read_and_reset(
	long long *o_count_values_read
) {
	if (!papi_counters_initialized)
		return;

	memset(o_count_values_read, 0,
		(size_t)papi_counters.event_list_len * sizeof(long long));

	for (int i = 0; i < papi_counters.num_event_sets; i++)
	{
		struct PapiEventSet *set = &papi_counters.event_sets[i];
		long long local_values[PERF_COUNTERS_MAX];

		if (PAPI_read(set->event_set, local_values) != PAPI_OK)
		{
			if (papi_counters.event_list_len != 0)
			{
				PAPI_perror("PAPI_read_counters: ");
				print_access_right_problems();
				exit(EXIT_FAILURE);
			}
			continue;
		}

		add_local_values_to_global(o_count_values_read, set, local_values);

		if (PAPI_reset(set->event_set) != PAPI_OK)
		{
			PAPI_perror("PAPI_reset_counters: ");
			print_access_right_problems();
			exit(EXIT_FAILURE);
		}
	}
}

#endif
