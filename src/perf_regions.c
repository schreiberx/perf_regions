#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <float.h>
#include <sys/time.h>
#include <string.h>


#include "perf_regions.h"
#include "papi_counters.h"

#include "perf_regions_defines.h"
#include "perf_regions_output.h"
// #include "perf_regions_names.h"

#define PRINT_PREFIX "[perf_regions.c] "

/*
 * precompiler directives to avoid compiling with timing or perf counters
 *
 * This gets handy if perf counter PAPI is not available on certain systems.
 */
#ifndef PERF_REGIONS_USE_PAPI
#	define PERF_REGIONS_USE_PAPI		1
#endif


/***********************************************
 * Activate support for nested performance counters.
 * Warning: This leads to additional overheads and
 * might result in decreased accurate results!
 *
 ***********************************************
 * Recursion not activated:
 *
 *  perf_region_start();
 *    -> count_start();
 *       (reset and start counters)
 *
 *  perf_region_stop();
 *    -> count_stop();
 *       (stop counters)
 *    -> accumulate performance counters
 *
 *
 ***********************************************
 * Recursion activated:
 *
 *  perf_region_init();
 *       (reset and start counters)
 *
 *  perf_region_start();
 *    if num_nested_performance_regions == 0:
 *        -> count_start();
 *
 *    else:
 *        -> count_read_and_reset();
 *           (read counters)
 *           (update performance counters of outer regions)
 *    num_nested_performance_regions++;
 *
 *
 *  perf_region_stop();
 *    num_nested_performance_regions--;
 *
 *    if num_nested_performance_regions == 0:
 *       -> count_stop();
 *          (stop counters)
 *
 *       -> accumulate performance counters
 *
 *    else:
 *       -> count_read_and_reset();
 *          (read counters)
 *          (update performance counters of outer regions)
 *
 *       -> accumulate performance counters		/// overheads included here for nested functions
 */
#ifndef PERF_COUNTERS_NESTED
#define PERF_COUNTERS_NESTED		1
#endif


/*
 * Use the higher-resolution POSIX clock rather than just gettimeofday()
 */
#ifndef PERF_TIMING_POSIX
#define PERF_TIMING_POSIX               1
#endif


#ifndef PERF_DEBUG
#	define PERF_DEBUG		1
#endif


#ifdef PERF_TIMING_POSIX
#include "posix_clock.h"
#endif

#ifndef USE_MPI
#define USE_MPI 1
#endif

#if USE_MPI
#include <mpi.h>
#endif

// Structs moved to perf_regions.h

struct PerfRegions perf_regions;


/**
 * reset the values in the performance regions
 */
void perf_regions_reset()
{
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(perf_regions.perf_regions_list[i]);

		r->region_name = 0;

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



/**
 * CONSTRUCTOR:
 *
 * Initialize the performance regions, e.g. allocate the data structures
 */
void perf_regions_init()
{
	if (getenv("PERF_REGIONS_VERBOSITY") == NULL) {
		perf_regions.verbosity = 0;
	} else {
		perf_regions.verbosity = atoi(getenv("PERF_REGIONS_VERBOSITY"));
	}

	if (perf_regions.verbosity > 0) {
		printf(PRINT_PREFIX"Verbosity level: %i\n", perf_regions.verbosity);
	}

	if (getenv("PERF_REGIONS_SKIP_N") == NULL) {
		perf_regions.skip_n = 0;
	} else {
		perf_regions.skip_n = atoi(getenv("PERF_REGIONS_SKIP_N"));
		if (perf_regions.verbosity > 0)
			printf(PRINT_PREFIX"PERF_REGIONS_SKIP_N: %i\n", perf_regions.skip_n);
	}

	perf_regions.perf_regions_list = 0;

	perf_regions.use_wallclock_time = 1;
#if PERF_REGIONS_USE_PAPI
	perf_regions.use_papi = 0;
#endif
	perf_regions.use_mpi = 0;

#if PERF_COUNTERS_NESTED
	perf_regions.num_nested_performance_regions = 0;
#endif


	/*
	 * CONSTRUCTOR
	 */
	const char *perf_counter_list;
	{
		if (getenv("PERF_REGIONS_COUNTERS") == NULL)
		{
			fprintf(stderr, "PERF_REGIONS_COUNTERS is not defined, using no performance counters\n");
			perf_counter_list = "";

			// activating only wallclock time measurements per default
			perf_regions.use_wallclock_time = 1;
		}
		else
		{
			perf_counter_list = getenv("PERF_REGIONS_COUNTERS");

			if (perf_regions.verbosity > 0)
				printf(PRINT_PREFIX"PERF_REGIONS_COUNTERS environment variable: '%s'\n", perf_counter_list);
			// disabling wallclock time measurements per default.
			// Can be activated by adding "WALLCLOCKTIME" to PERF_REGIONS_COUNTERS
			perf_regions.use_wallclock_time = 0;
		}
	}

	/*
	 * Duplicate string for tokenization of list
	 */
	char *perf_counter_list_tokens = strdup(perf_counter_list);
	perf_regions.num_perf_counters = 0;

	// Get comma-separated events
	char *event = strtok(perf_counter_list_tokens, ",");
	while (event != NULL)
	{
		if (strcmp("WALLCLOCKTIME", event) == 0)
		{
			perf_regions.use_wallclock_time = 1;
		}
		else
		{
#if PERF_REGIONS_USE_PAPI

			perf_regions.perf_counter_names[perf_regions.num_perf_counters] = strdup(event);
			perf_regions.num_perf_counters++;

			perf_regions.use_papi = 1;
#else
			fprintf(stderr, "PAPI performance counters not supported in this build, but PERF_REGIONS_COUNTERS requests '%s'\n", event);
			exit(-1);
#endif
		}

		event = strtok(NULL, ",");
	}

#if PERF_REGIONS_USE_PAPI
	papi_counters_init(perf_regions.perf_counter_names, perf_regions.num_perf_counters, perf_regions.verbosity);
#endif

	/*
	 * Output configuration
	 */
	perf_regions.output_console = 1;
	perf_regions.output_json = 0;
	perf_regions.output_json_filename = NULL;

	char* env_output = getenv("PERF_REGIONS_OUTPUT");
	if (env_output != NULL)
	{
		perf_regions.output_console = 0; // If variable is set, disable default unless explicitly enabled

		char *output_tokens = strdup(env_output);
		char *token = strtok(output_tokens, ",");
		while (token != NULL)
		{
			if (strcmp(token, "console") == 0)
			{
				perf_regions.output_console = 1;
			}
			else if (strncmp(token, "json", 4) == 0)
			{
				perf_regions.output_json = 1;

				if (strlen(token) > 5 && token[4] == '=')
				{
					perf_regions.output_json_filename = strdup(token + 5);
				}
			}
			token = strtok(NULL, ",");
		}
		free(output_tokens);
	}

	free(perf_counter_list_tokens);

	if (perf_regions.perf_regions_list != NULL)
	{
		fprintf(stderr, "perf_regions_init called twice?");
		exit(-1);
	}

	perf_regions.perf_regions_list = malloc(sizeof(struct PerfRegion)*(PERF_REGIONS_MAX));


	/*
	 * RESET
	 */
	perf_regions_reset();
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
	if(perf_regions.verbosity > 0) {
		printf(PRINT_PREFIX"Using MPI communicator %d for performance regions\n", communicator);
	}
	perf_regions_init_mpi(comm);
}

#endif

/**
 * start measuring the performance for region given by the ID.
 */
void perf_region_start(
	int i_region_id,	///< id of region
	const char* i_region_name	///< region name (associated with ID)
)
{
	struct PerfRegion *r = &(perf_regions.perf_regions_list[i_region_id]);

	// Initialize only once
	// There's a small overhead, but we hope that this is negligible since we also do this before starting performance counting
	if (r->region_name == 0)
		r->region_name = strdup(i_region_name);

	//
	// SKIPPING LOGIC
	//
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

	if (i_region_id < 0 || i_region_id >= PERF_REGIONS_MAX)
	{
		fprintf(stderr, "Region ID %i out of boundaries\n", i_region_id);
		exit(1);
	}

	if (r->active > 0)
	{
		fprintf(stderr, "Region activated twice\n");
		exit(1);
	}

	r->active = 1;

#endif

#if PERF_COUNTERS_NESTED
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
				// outer performance region
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
#ifdef PERF_TIMING_POSIX
    r->region_enter_time_posix = posix_clock();
#else
    gettimeofday(&(r->start_time_value), NULL);
#endif
}


/**
 * This is called from Fortran where also the length of the region name is
 * handed over. We'll have additional overheads therefore for Fortran, but
 * that's how it goes with Fortran.
 */
void perf_region_start_fortran(
	int i_region_id,	///< id of region
	const char* i_region_name,	///< region name (associated with ID)
	size_t len ///< length of region_name
)
{
	struct PerfRegion *r = &(perf_regions.perf_regions_list[i_region_id]);

	if (r->region_name == 0)
	{
		r->region_name = (char *)malloc((len + 1) * sizeof(char));
		strncpy(r->region_name, i_region_name, len);
		r->region_name[len] = '\0';
	}

	perf_region_start(i_region_id, 0);
}



/**
 * stop measuring the performance for region given by the ID
 */
void perf_region_stop(
	int i_region_id		///< unique id of region
)
{
	struct PerfRegion *r = &perf_regions.perf_regions_list[i_region_id];

	if (r->current_execution_skipped)
	{
#if PERF_DEBUG
		r->active = 0;
#endif
#if PERF_COUNTERS_NESTED
		if (perf_regions.num_nested_performance_regions > 0)
			perf_regions.num_nested_performance_regions--;
#endif
		return;
	}

#if PERF_DEBUG
	if (r->active != 1)
	{
		printf("Region not active, but stop function called\n");
		exit(1);
	}
	if (perf_regions.num_nested_performance_regions == 0)
	{
		printf("More stops than starts detected\n");
		exit(1);
	}

	r->active = 0;
#endif


#if PERF_COUNTERS_NESTED
	perf_regions.num_nested_performance_regions--;
#endif

#if PERF_REGIONS_USE_PAPI
	if (perf_regions.use_papi)
	{
#  if PERF_COUNTERS_NESTED

		long long counter_values_stop[PERF_COUNTERS_MAX];

		if (perf_regions.num_nested_performance_regions == 0)
		{
#if PERF_DEBUG
			if (r != perf_regions.nested_performance_regions[0])
			{
				printf("MISMATCH in nested performance regions on stop\n");
				exit(-1);
			}
#endif

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
				// outer performance region
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
#ifdef PERF_TIMING_POSIX
		double last_wallclock_time_measurement = posix_clock() - r->region_enter_time_posix;
#else
		struct timeval tm2;
		gettimeofday(&tm2, NULL);
		double last_wallclock_time_measurement = ((double)time_val.tv_sec - (double)r->start_time_value.tv_sec) + ((double)time_val.tv_usec - (double)r->start_time_value.tv_usec) * 0.000001;
#endif

		r->counter_wallclock_time += last_wallclock_time_measurement;
		r->max_wallclock_time = r->max_wallclock_time > last_wallclock_time_measurement ? r->max_wallclock_time : last_wallclock_time_measurement;
		r->min_wallclock_time = r->min_wallclock_time < last_wallclock_time_measurement ? r->min_wallclock_time : last_wallclock_time_measurement;
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
void perf_regions_finalize()
{
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
		/* output performance information on each region to console */
		if (perf_regions.output_console)
			perf_regions_output_human_readable_text();

		if (perf_regions.output_json)
			perf_regions_output_json_file(perf_regions.output_json_filename);

	}
#else

/* output performance information on each region to console */
	if (perf_regions.output_console)
		perf_regions_output_human_readable_text();

	if (perf_regions.output_json)
		perf_regions_output_json_file(perf_regions.output_json_filename);
#endif

	/*
	 * Output .csv file
	 */
	perf_regions_output_csv_file();


	/*
	 * DECONSTRUCTUR
	 */
	if (perf_regions.perf_regions_list != NULL)	{
		for (int i = 0; i < PERF_REGIONS_MAX; i++) {
			if (perf_regions.perf_regions_list[i].region_name != 0)
				free(perf_regions.perf_regions_list[i].region_name);
		}

		free(perf_regions.perf_regions_list);
		perf_regions.perf_regions_list = NULL;
	}

#ifndef PERF_REGIONS_USE_PAPI
	papi_counters_finalize();

	for (int i = 0; i < perf_regions.num_perf_counters; i++)
		free(perf_regions.perf_counter_names[i]);
#endif

#if PERF_COUNTERS_NESTED
	if (perf_regions.num_nested_performance_regions != 0)
	{
		fprintf(stderr, "num_nested_performance_regions != 0");
		exit(-1);
	}
#endif
}

