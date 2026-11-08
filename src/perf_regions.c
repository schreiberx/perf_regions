#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <float.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "perf_regions.h"
#include "perf_regions_internal.h"
#include "papi_counters.h"
#include "perf_regions_defines.h"
#include "perf_regions_output.h"

#define PRINT_PREFIX "[perf_regions.c] "

#if PERF_TIMING_POSIX
#include "posix_clock.h"
#endif

#if USE_MPI
#include <mpi.h>
#endif

struct PerfRegions perf_regions;

/* Is the library currently initialized? */
static int perf_regions_initialized = 0;

static char *perf_regions_strdup(const char *s)
{
	if (s == NULL)
		return NULL;

	size_t len = strlen(s) + 1;
	char *copy = (char *)malloc(len);
	if (copy == NULL)
	{
		fprintf(stderr, PRINT_PREFIX"Out of memory\n");
		exit(EXIT_FAILURE);
	}

	memcpy(copy, s, len);
	return copy;
}

/*
 * Store the region name only on the first start of a region ID.  This helper
 * is shared by the C and Fortran entry points so both keep the same
 * "copy once" semantics.
 */
static void perf_region_store_name_once(
	struct PerfRegion *r,
	int i_region_id,
	const char *i_region_name,
	size_t len
)
{
	if (r->region_name != NULL)
		return;

	if (i_region_name == NULL)
	{
		fprintf(stderr, PRINT_PREFIX"Region %i started without a name\n", i_region_id);
		exit(EXIT_FAILURE);
	}

	char *copy = (char *)malloc(len + 1);
	if (copy == NULL)
	{
		fprintf(stderr, PRINT_PREFIX"Out of memory\n");
		exit(EXIT_FAILURE);
	}

	memcpy(copy, i_region_name, len);
	copy[len] = '\0';
	r->region_name = copy;
}

static void perf_regions_trim(char *s)
{
	if (s == NULL)
		return;

	char *start = s;
	while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')
		start++;

	if (start != s)
		memmove(s, start, strlen(start) + 1);

	size_t len = strlen(s);
	while (len > 0)
	{
		char c = s[len - 1];
		if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
			break;
		s[--len] = '\0';
	}
}

static struct PerfRegion *perf_region_get_checked(int i_region_id)
{
	if (i_region_id < 0 || i_region_id >= PERF_REGIONS_MAX)
	{
		fprintf(stderr, PRINT_PREFIX"Region ID %i out of boundaries (valid: 0..%i)\n",
			i_region_id, PERF_REGIONS_MAX - 1);
		exit(EXIT_FAILURE);
	}

	if (perf_regions.perf_regions_list == NULL)
	{
		fprintf(stderr, PRINT_PREFIX"perf_regions_init() has not been called\n");
		exit(EXIT_FAILURE);
	}

	return &(perf_regions.perf_regions_list[i_region_id]);
}


/**
 * reset the values in the performance regions
 */
static void perf_regions_reset(void)
{
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(perf_regions.perf_regions_list[i]);

		if (r->region_name != NULL)
		{
			free(r->region_name);
			r->region_name = NULL;
		}

		r->current_execution_skipped = 0;

#if PERF_REGIONS_USE_PAPI
		for (int j = 0; j < perf_regions.num_perf_counters; j++)
			r->counter_values[j] = 0;
#endif

		r->counter_wallclock_time = 0;
		r->region_enter_counter = 0;
		r->region_skipped_counter = 0;
		r->max_wallclock_time = 0;
		r->min_wallclock_time = DBL_MAX;
		r->squared_dist_wallclock_time = 0;
		r->running_mean = 0;

#if PERF_COUNTERS_NESTED
		r->spoiled = 0;
#endif

#if PERF_DEBUG
		perf_regions.perf_regions_list[i].active = 0;
#endif
	}

#if PERF_COUNTERS_NESTED
	perf_regions.num_nested_performance_regions = 0;
#endif
}



static int perf_regions_env_int(const char *name, int fallback)
{
	const char *value = getenv(name);
	if (value == NULL || *value == '\0')
		return fallback;

	errno = 0;
	char *end = NULL;
	long parsed = strtol(value, &end, 10);
	if (errno != 0 || end == value || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX)
	{
		fprintf(stderr, PRINT_PREFIX"Ignoring invalid value for %s: '%s'\n", name, value);
		return fallback;
	}

	return (int)parsed;
}

/**
 * CONSTRUCTOR:
 *
 * Initialize the performance regions, e.g. allocate the data structures
 */
void perf_regions_init(void)
{
	if (perf_regions_initialized)
	{
		if (perf_regions.verbosity > 0)
			printf(PRINT_PREFIX"perf_regions_init called twice; ignoring\n");
		return;
	}

	memset(&perf_regions, 0, sizeof(perf_regions));

	perf_regions.verbosity = perf_regions_env_int("PERF_REGIONS_VERBOSITY", 0);
	if (perf_regions.verbosity > 0)
		printf(PRINT_PREFIX"Verbosity level: %i\n", perf_regions.verbosity);

	perf_regions.skip_n = perf_regions_env_int("PERF_REGIONS_SKIP_N", 0);
	if (perf_regions.skip_n < 0)
		perf_regions.skip_n = 0;
	if (perf_regions.verbosity > 0)
		printf(PRINT_PREFIX"PERF_REGIONS_SKIP_N: %i\n", perf_regions.skip_n);

#if PERF_TIMING_POSIX
	double clock_resolution = 0.0;
	perf_regions.use_posix_clock = posix_clock_init(&clock_resolution);
	if (perf_regions.verbosity > 0)
	{
		if (perf_regions.use_posix_clock)
			printf(PRINT_PREFIX"POSIX clock resolution: %.3e s\n", clock_resolution);
		else
			printf(PRINT_PREFIX"POSIX timers unavailable, falling back to gettimeofday\n");
	}
#endif

	perf_regions.use_wallclock_time = 1;
#if PERF_REGIONS_USE_PAPI
	perf_regions.use_papi = 0;
#endif
	perf_regions.use_mpi = 0;

#if PERF_COUNTERS_NESTED
	perf_regions.num_nested_performance_regions = 0;
#endif

	/*
	 * Parse the counter list.  If PERF_REGIONS_COUNTERS is unset, keep the
	 * default wallclock timing enabled.
	 */
	const char *env_counters = getenv("PERF_REGIONS_COUNTERS");
	const char *perf_counter_list = (env_counters == NULL) ? "" : env_counters;

	if (env_counters == NULL)
	{
		if (perf_regions.verbosity > 0)
			fprintf(stderr, PRINT_PREFIX"PERF_REGIONS_COUNTERS is not defined, using wallclock time only\n");
	}
	else
	{
		if (perf_regions.verbosity > 0)
			printf(PRINT_PREFIX"PERF_REGIONS_COUNTERS environment variable: '%s'\n", perf_counter_list);

		// Disable wallclock timing by default.  It can be activated by adding
		// "WALLCLOCKTIME" to PERF_REGIONS_COUNTERS.
		perf_regions.use_wallclock_time = 0;
	}

	char *perf_counter_list_tokens = perf_regions_strdup(perf_counter_list);
	perf_regions.num_perf_counters = 0;

	char *saveptr = NULL;
	for (char *event = strtok_r(perf_counter_list_tokens, ",", &saveptr);
	     event != NULL;
	     event = strtok_r(NULL, ",", &saveptr))
	{
		perf_regions_trim(event);
		if (*event == '\0')
			continue;

		if (strcmp("WALLCLOCKTIME", event) == 0)
		{
			perf_regions.use_wallclock_time = 1;
		}
		else
		{
#if PERF_REGIONS_USE_PAPI
			if (perf_regions.num_perf_counters >= PERF_COUNTERS_MAX)
			{
				fprintf(stderr, PRINT_PREFIX"Too many performance counters requested (max: %i)\n",
					PERF_COUNTERS_MAX);
				exit(EXIT_FAILURE);
			}

			perf_regions.perf_counter_names[perf_regions.num_perf_counters] =
				perf_regions_strdup(event);
			perf_regions.num_perf_counters++;

			perf_regions.use_papi = 1;
#else
			fprintf(stderr, PRINT_PREFIX"PAPI performance counters not supported in this build, "
				"but PERF_REGIONS_COUNTERS requests '%s'\n", event);
			exit(EXIT_FAILURE);
#endif
		}
	}

	free(perf_counter_list_tokens);

#if PERF_REGIONS_USE_PAPI
	// Do not initialize PAPI for wallclock-only runs.  This keeps a
	// PAPI-enabled build usable on systems where the PAPI runtime is missing.
	if (perf_regions.num_perf_counters > 0)
		papi_counters_init(perf_regions.perf_counter_names,
			perf_regions.num_perf_counters, perf_regions.verbosity);
#endif

	/*
	 * Output configuration
	 */
	perf_regions.output_console = 1;
	perf_regions.output_json = 0;
	perf_regions.output_json_filename = NULL;

	char *env_output = getenv("PERF_REGIONS_OUTPUT");
	if (env_output != NULL)
	{
		// If the variable is set, disable the default unless explicitly enabled.
		perf_regions.output_console = 0;

		char *output_tokens = perf_regions_strdup(env_output);
		char *output_saveptr = NULL;
		for (char *token = strtok_r(output_tokens, ",", &output_saveptr);
		     token != NULL;
		     token = strtok_r(NULL, ",", &output_saveptr))
		{
			perf_regions_trim(token);
			if (*token == '\0')
				continue;

			if (strcmp(token, "console") == 0)
			{
				perf_regions.output_console = 1;
			}
			else if (strcmp(token, "json") == 0)
			{
				perf_regions.output_json = 1;
			}
			else if (strncmp(token, "json=", 5) == 0)
			{
				perf_regions.output_json = 1;
				const char *filename = token + 5;
				if (*filename != '\0')
				{
					free(perf_regions.output_json_filename);
					perf_regions.output_json_filename = perf_regions_strdup(filename);
				}
			}
			else
			{
				fprintf(stderr, PRINT_PREFIX"Ignoring unknown PERF_REGIONS_OUTPUT token '%s'\n", token);
			}
		}
		free(output_tokens);
	}

	if (perf_regions.verbosity > 0)
	{
		printf(PRINT_PREFIX"PERF_REGIONS_OUTPUT: console=%i json=%i file=%s\n",
			perf_regions.output_console,
			perf_regions.output_json,
			perf_regions.output_json_filename != NULL ? perf_regions.output_json_filename : "(stdout)");
	}

	perf_regions.perf_regions_list = (struct PerfRegion *)calloc(
		PERF_REGIONS_MAX, sizeof(struct PerfRegion));
	if (perf_regions.perf_regions_list == NULL)
	{
		fprintf(stderr, PRINT_PREFIX"Failed to allocate performance regions\n");
		exit(EXIT_FAILURE);
	}

	/*
	 * RESET
	 */
	perf_regions_reset();
	perf_regions_initialized = 1;
}

#if USE_MPI
void perf_regions_init_mpi(MPI_Comm communicator)
{
	perf_regions_init();
	perf_regions.use_mpi = 1;
	perf_regions.comm = communicator;
}

void perf_regions_init_mpi_fortran(int communicator)
{
	// Convert Fortran MPI communicator to C MPI communicator
	MPI_Comm comm = MPI_Comm_f2c((MPI_Fint)communicator);
	if(comm == MPI_COMM_NULL) {
		fprintf(stderr, "Invalid MPI communicator passed to perf_regions_init_mpi_fortran\n");
		exit(EXIT_FAILURE);
	}
	perf_regions_init_mpi(comm);
	if (perf_regions.verbosity > 0)
		printf(PRINT_PREFIX"Using MPI communicator %d for performance regions\n", communicator);
}

#endif

/**
 * start measuring the performance for region given by the ID.
 */
void perf_region_start(
	int i_region_id,
	const char *i_region_name
)
{
	struct PerfRegion *r = perf_region_get_checked(i_region_id);

	/* Initialize the name only once. */
	size_t name_len = i_region_name != NULL ? strlen(i_region_name) : 0;
	perf_region_store_name_once(r, i_region_id, i_region_name, name_len);

	/*
	 * SKIPPING LOGIC
	 */
	r->current_execution_skipped = 0;
#if PERF_COUNTERS_NESTED
	if (perf_regions.num_nested_performance_regions > 0)
	{
		// Check whether parent region is already skipped, if yes, skip current region as well
		struct PerfRegion *parent = perf_regions.nested_performance_regions[perf_regions.num_nested_performance_regions - 1];
		if (parent->current_execution_skipped)
			r->current_execution_skipped = 1;
	}
#endif

	// check for skip N
	if (r->current_execution_skipped == 0)
	{
		if (r->region_enter_counter < perf_regions.skip_n)
			r->current_execution_skipped = 1;
	}

	r->region_enter_counter++;

	if (r->current_execution_skipped)
		r->region_skipped_counter++;

#if PERF_DEBUG
	if (r->active > 0)
	{
		fprintf(stderr, PRINT_PREFIX"Region '%s' activated twice\n",
			r->region_name != NULL ? r->region_name : "(null)");
		exit(EXIT_FAILURE);
	}

	r->active = 1;
#endif

#if PERF_COUNTERS_NESTED
	if (perf_regions.num_nested_performance_regions >= PERF_NESTED_REGIONS_MAX)
	{
		fprintf(stderr, PRINT_PREFIX"Maximum nested region depth (%i) exceeded\n",
			PERF_NESTED_REGIONS_MAX);
		exit(EXIT_FAILURE);
	}

	perf_regions.nested_performance_regions[perf_regions.num_nested_performance_regions] = r;
#endif

	if (r->current_execution_skipped)
	{
#if PERF_COUNTERS_NESTED
		perf_regions.num_nested_performance_regions++;
#endif
		return;
	}

#if PERF_REGIONS_USE_PAPI
	if (perf_regions.use_papi)
	{
#  if PERF_COUNTERS_NESTED

		if (perf_regions.num_nested_performance_regions == 0)
		{
			papi_counters_start();
		}
		else
		{
			long long counter_values_stop[PERF_COUNTERS_MAX];

			papi_counters_read_and_reset(counter_values_stop);

			// add performance values to outer nested performance regions
			for (int i = 0; i < perf_regions.num_nested_performance_regions; i++)
			{
				struct PerfRegion *r2 = perf_regions.nested_performance_regions[i];

				for (int j = 0; j < perf_regions.num_perf_counters; j++)
					r2->counter_values[j] += counter_values_stop[j];
			}
		}

#  else
		papi_counters_start();
#  endif
	}
#endif

#if PERF_COUNTERS_NESTED
	perf_regions.num_nested_performance_regions++;
#endif

	if (perf_regions.use_wallclock_time)
	{
#if PERF_TIMING_POSIX
		if (perf_regions.use_posix_clock)
			r->region_enter_time_posix = posix_clock();
		else
			gettimeofday(&(r->start_time_value), NULL);
#else
		gettimeofday(&(r->start_time_value), NULL);
#endif
	}
}


/**
 * This is called from Fortran where also the length of the region name is
 * handed over. We'll have additional overheads therefore for Fortran, but
 * that's how it goes with Fortran.
 */
void perf_region_start_fortran(
	int i_region_id,
	const char *i_region_name,
	size_t len
)
{
	struct PerfRegion *r = perf_region_get_checked(i_region_id);

	/*
	 * Copy the Fortran string only on the first start of this region.
	 * Subsequent starts with the same ID must not allocate or copy it again.
	 */
	perf_region_store_name_once(r, i_region_id, i_region_name, len);

	perf_region_start(i_region_id, NULL);
}


/**
 * stop measuring the performance for region given by the ID
 */
void perf_region_stop(
	int i_region_id
)
{
	struct PerfRegion *r = perf_region_get_checked(i_region_id);

	if (r->current_execution_skipped)
	{
#if PERF_COUNTERS_NESTED
		if (perf_regions.num_nested_performance_regions == 0 ||
		    perf_regions.nested_performance_regions[perf_regions.num_nested_performance_regions - 1] != r)
		{
			fprintf(stderr, PRINT_PREFIX"Unbalanced stop for skipped region '%s'\n",
				r->region_name != NULL ? r->region_name : "(null)");
			exit(EXIT_FAILURE);
		}
		perf_regions.num_nested_performance_regions--;
#endif
#if PERF_DEBUG
		r->active = 0;
#endif
		return;
	}

#if PERF_DEBUG
	if (r->active != 1)
	{
		fprintf(stderr, PRINT_PREFIX"Region '%s' not active, but stop function called\n",
			r->region_name != NULL ? r->region_name : "(null)");
		exit(EXIT_FAILURE);
	}
#endif

#if PERF_COUNTERS_NESTED
	if (perf_regions.num_nested_performance_regions == 0 ||
	    perf_regions.nested_performance_regions[perf_regions.num_nested_performance_regions - 1] != r)
	{
		fprintf(stderr, PRINT_PREFIX"MISMATCH in nested performance regions on stop\n");
		exit(EXIT_FAILURE);
	}
	perf_regions.num_nested_performance_regions--;
#endif

#if PERF_DEBUG
	r->active = 0;
#endif

#if PERF_REGIONS_USE_PAPI
	if (perf_regions.use_papi)
	{
#  if PERF_COUNTERS_NESTED

		long long counter_values_stop[PERF_COUNTERS_MAX];

		if (perf_regions.num_nested_performance_regions == 0)
		{
			papi_counters_stop(counter_values_stop);

			for (int j = 0; j < perf_regions.num_perf_counters; j++)
				r->counter_values[j] += counter_values_stop[j];
		}
		else
		{
			papi_counters_read_and_reset(counter_values_stop);

			// add performance values to outer nested AND CURRENT performance region
			for (int i = 0; i <= perf_regions.num_nested_performance_regions; i++)
			{
				struct PerfRegion *r2 = perf_regions.nested_performance_regions[i];

				for (int j = 0; j < perf_regions.num_perf_counters; j++)
					r2->counter_values[j] += counter_values_stop[j];

				if (i < perf_regions.num_nested_performance_regions)
					r2->spoiled = 1;
			}
		}

#  else

		long long counter_values_stop[PERF_COUNTERS_MAX];
		papi_counters_stop(counter_values_stop);

		for (int j = 0; j < perf_regions.num_perf_counters; j++)
			r->counter_values[j] += counter_values_stop[j];

#  endif
	}
#endif

	if (perf_regions.use_wallclock_time)
	{
		struct timeval tm2;
		double last_wallclock_time_measurement;

#if PERF_TIMING_POSIX
		if (perf_regions.use_posix_clock)
		{
			last_wallclock_time_measurement = posix_clock() - r->region_enter_time_posix;
		}
		else
		{
			gettimeofday(&tm2, NULL);
			last_wallclock_time_measurement =
				((double)tm2.tv_sec - (double)r->start_time_value.tv_sec) +
				((double)tm2.tv_usec - (double)r->start_time_value.tv_usec) * 0.000001;
		}
#else
		gettimeofday(&tm2, NULL);
		last_wallclock_time_measurement =
			((double)tm2.tv_sec - (double)r->start_time_value.tv_sec) +
			((double)tm2.tv_usec - (double)r->start_time_value.tv_usec) * 0.000001;
#endif

		r->counter_wallclock_time += last_wallclock_time_measurement;
		r->max_wallclock_time = r->max_wallclock_time > last_wallclock_time_measurement
			? r->max_wallclock_time : last_wallclock_time_measurement;
		r->min_wallclock_time = r->min_wallclock_time < last_wallclock_time_measurement
			? r->min_wallclock_time : last_wallclock_time_measurement;

		// Variance with Welford's online algorithm
		double delta = last_wallclock_time_measurement - r->running_mean;
		double valid_samples = r->region_enter_counter - r->region_skipped_counter;
		if (valid_samples > 0)
			r->running_mean += delta / valid_samples;
		r->squared_dist_wallclock_time += delta * (last_wallclock_time_measurement - r->running_mean);
	}
}


/**
 * DECONSTRUCTOR
 */
void perf_regions_finalize(void)
{
	if (!perf_regions_initialized)
		return;

#if USE_MPI
	// Use MPI to generate statistics and output these and the result for rank 0
	if (perf_regions.use_mpi)
	{
		if (perf_regions.output_console)
			reduce_and_output_human_readable_text();

		if (perf_regions.output_json)
			reduce_and_output_json_file(perf_regions.output_json_filename);
	}
	else
	{
		if (perf_regions.output_console)
			perf_regions_output_human_readable_text();

		if (perf_regions.output_json)
			perf_regions_output_json_file(perf_regions.output_json_filename);
	}
#else
	if (perf_regions.output_console)
		perf_regions_output_human_readable_text();

	if (perf_regions.output_json)
		perf_regions_output_json_file(perf_regions.output_json_filename);
#endif

#if PERF_COUNTERS_NESTED
	if (perf_regions.num_nested_performance_regions != 0)
	{
		fprintf(stderr, PRINT_PREFIX"num_nested_performance_regions != 0 (%i); "
			"regions were started but not stopped\n",
			perf_regions.num_nested_performance_regions);
	}
#endif

#if PERF_REGIONS_USE_PAPI
	if (perf_regions.use_papi)
		papi_counters_finalize();

	for (int i = 0; i < perf_regions.num_perf_counters; i++)
	{
		free(perf_regions.perf_counter_names[i]);
		perf_regions.perf_counter_names[i] = NULL;
	}
#endif

	/*
	 * DECONSTRUCTOR
	 */
	if (perf_regions.perf_regions_list != NULL)
	{
		for (int i = 0; i < PERF_REGIONS_MAX; i++)
			free(perf_regions.perf_regions_list[i].region_name);

		free(perf_regions.perf_regions_list);
		perf_regions.perf_regions_list = NULL;
	}

	free(perf_regions.output_json_filename);
	perf_regions.output_json_filename = NULL;
	perf_regions.output_console = 0;
	perf_regions.output_json = 0;
	perf_regions_initialized = 0;
}
