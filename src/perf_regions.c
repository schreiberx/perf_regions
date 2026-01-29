#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <float.h>
#include <sys/time.h>
#include <string.h>


#include "perf_regions.h"
#include "papi_counters.h"

#include "perf_regions_defines.h"
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

/**
 * Structure for each of the performance regions
 */
struct PerfRegion
{
#if PERF_REGIONS_USE_PAPI
	// storage for performance counter
	long long counter_values[PERF_COUNTERS_MAX];
#endif

	// accumulator to get total wallclock time spent in this region and its variance (over time)
	double counter_wallclock_time;
	double squared_dist_wallclock_time;
	double min_wallclock_time;
	double max_wallclock_time;
	double running_mean;

#ifdef PERF_TIMING_POSIX
	// Start time value (seconds)
	double region_enter_time_posix;
#else
	// time value
	struct timeval start_time_value;
#endif

	// region enter counter to normalize the performance counters
	double region_enter_counter;

#if PERF_COUNTERS_NESTED
	long long count_values_read[PERF_COUNTERS_MAX];

	// Set to true if performance counters could be spoiled due to nested
	int spoiled;
#endif

#if PERF_DEBUG
	// debug flag to assure that perf counters are not entering a region twice
	int active;
#endif

	// Region name
	char* region_name;
};


struct PerfRegions
{
	// verbosity level
	int verbosity;

	// array with all regions
	struct PerfRegion *perf_regions_list;

	// Measure wallclock time
	int use_wallclock_time;

	// Measure with PAPI
	int use_papi;

	// number of performance counters
	int num_perf_counters;

	// names of performance counters
	char *perf_counter_names[PERF_COUNTERS_MAX];


#if PERF_COUNTERS_NESTED
	// number of nested performance regions
	int num_nested_performance_regions;

	// number of maximal nested performance regions
	#define PERF_NESTED_REGIONS_MAX 16

	struct PerfRegion *nested_performance_regions[PERF_NESTED_REGIONS_MAX];
#endif

	// the application uses MPI / perf_regions will be called for MPI processes
	// we want to do reduction at the end for the result
	int use_mpi;
#if USE_MPI
	MPI_Comm comm;
#endif

} perf_regions;


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

	perf_regions.perf_regions_list = 0;

	perf_regions.use_wallclock_time = 1;
	perf_regions.use_papi = 0;
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
			perf_regions.perf_counter_names[perf_regions.num_perf_counters] = strdup(event);
			perf_regions.num_perf_counters++;

			perf_regions.use_papi = 1;
		}

		event = strtok(NULL, ",");
	}

#if PERF_REGIONS_USE_PAPI
	papi_counters_init(perf_regions.perf_counter_names, perf_regions.num_perf_counters, perf_regions.verbosity);
#endif

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

	// denominator to normalize values
	r->region_enter_counter++;

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

#if PERF_REGIONS_USE_PAPI
	if (perf_regions.use_papi)
	{
#  if PERF_COUNTERS_NESTED

		perf_regions.nested_performance_regions[perf_regions.num_nested_performance_regions] = r;

		if (perf_regions.num_nested_performance_regions == 0)
		{
			papi_counters_start();
		}
		else
		{
			papi_counters_read_and_reset(r->count_values_read);

			// add performance values to outer nested performance regions
			for (int i = 0; i < perf_regions.num_nested_performance_regions; i++)
			{
				// outer performance region
				struct PerfRegion *r_outer = perf_regions.nested_performance_regions[i];

				for (int j = 0; j < perf_regions.num_perf_counters; j++)
					r_outer->counter_values[j] += r->count_values_read[j];
			}
		}

		perf_regions.num_nested_performance_regions++;

#  else
		papi_counters_start();
#  endif
	}


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

#if PERF_DEBUG
	if (r->active != 1)
	{
		printf("Region not active, but stop function called\n");
		exit(1);
	}

	r->active = 0;
#endif


#if PERF_REGIONS_USE_PAPI
	if (perf_regions.use_papi)
	{
#  if PERF_COUNTERS_NESTED

		perf_regions.num_nested_performance_regions--;

		if (perf_regions.num_nested_performance_regions == 0)
		{
			long long counter_values_stop[PERF_COUNTERS_MAX];
			papi_counters_stop(counter_values_stop);

			for (int j = 0; j < perf_regions.num_perf_counters; j++)
				r->counter_values[j] += counter_values_stop[j];
		}
		else
		{
			papi_counters_read_and_reset(r->count_values_read);

			// add performance values to outer nested AND CURRENT performance region
			for (int i = 0; i <= perf_regions.num_nested_performance_regions; i++)
			{
				// outer performance region
				struct PerfRegion *r_outer = perf_regions.nested_performance_regions[i];

				for (int j = 0; j < perf_regions.num_perf_counters; j++)
					r_outer->counter_values[j] += r->count_values_read[j];

				if (i < perf_regions.num_nested_performance_regions)
					r_outer->spoiled = 1;
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
		r->running_mean += delta / r->region_enter_counter;
		r->squared_dist_wallclock_time += delta * (last_wallclock_time_measurement - r->running_mean);
	}
}



void perf_regions_output_human_readable_text()
{

	FILE *s = stdout;

	fprintf(s, "[MULE] perf_regions: Performance counters profiling:\n");
	fprintf(s, "[MULE] perf_regions: ----------------------\n");
	fprintf(s, "[MULE] perf_regions: Section");
	for (int j = 0; j < perf_regions.num_perf_counters; j++)
		fprintf(s, "\t%s", perf_regions.perf_counter_names[j]);
#  if PERF_COUNTERS_NESTED
	fprintf(s, "\tSPOILED");
#  endif
	if (perf_regions.use_wallclock_time)
	{
		fprintf(s, "\tWALLCLOCKTIME");
		fprintf(s, "\tMIN");
		fprintf(s, "\tMAX");
		fprintf(s, "\tMEAN");
		fprintf(s, "\tVAR");
	}
	fprintf(s, "\tCOUNTER");
	fprintf(s, "\n");

#  if PERF_DEBUG
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(perf_regions.perf_regions_list[i]);
		if (r->active != 0)
		{
			if (r->region_name != 0)
				fprintf(stderr, "Still in region of %s\n", r->region_name);
			else
				fprintf(stderr, "Still in region NULL\n");

			exit(-1);
		}
	}
#  endif

	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(perf_regions.perf_regions_list[i]);

		if (r->region_name == 0)
			continue;

		fprintf(s, "[MULE] perf_regions: %s", r->region_name);


#if PERF_REGIONS_USE_PAPI
		for (int j = 0; j < perf_regions.num_perf_counters; j++)
		{
			long long counter_value = r->counter_values[j];

			double param_value = counter_value;

			fprintf(s, "\t%.7e", param_value);
		}
#endif

#  if PERF_COUNTERS_NESTED
		fprintf(s, "\t%i", r->spoiled);
#  endif
		if (perf_regions.use_wallclock_time) {
			fprintf(s, "\t%.7e", r->counter_wallclock_time);
			fprintf(s, "\t%.7e", r->min_wallclock_time);
			fprintf(s, "\t%.7e", r->max_wallclock_time);
			fprintf(s, "\t%.7e", r->running_mean);
			fprintf(s, "\t%.7e", r->squared_dist_wallclock_time / r->region_enter_counter);
		}

		fprintf(s, "\t%.0f", r->region_enter_counter);

		fprintf(s, "\n");
	}
}


void perf_regions_output_csv_file()
{

}

void reduce_and_output_human_readable_text()
{
	/* Reduces and aggregates performance region statistics across MPI processes.
	 * This function performs MPI reductions (min, max, sum/average) on wallclock time and performance counters
	 * for all regions, and outputs the results on the root process.
	 */
	if (!perf_regions.use_wallclock_time)
		return;
#if USE_MPI
	int rank, size;
	MPI_Comm_rank(perf_regions.comm, &rank);
	MPI_Comm_size(perf_regions.comm, &size);

#  if PERF_REGIONS_USE_PAPI

	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(perf_regions.perf_regions_list[i]);
		if (r->region_name == 0)
			continue;

		double *recv_buf = NULL;
		if (rank == 0)
		{
			// we might have a large number of processes
			recv_buf = (double *)malloc(size * 3 * sizeof(double));
			if (!recv_buf)
			{
				fprintf(stderr, "Failed to allocate recv_buf\n");
				exit(-1);
			}
		}
		double send_buf[3] = {r->region_enter_counter, r->running_mean, r->squared_dist_wallclock_time};
		MPI_Gather(send_buf, 3, MPI_DOUBLE, recv_buf, 3, MPI_DOUBLE, 0, perf_regions.comm);

		// Calculate mean and variance of wallclock time across all processes
		// use the Welford/Chan parallel algorithm so it's numerically stable
		if (rank == 0)
		{
			double n = 0.0;
			double na, nb;
			double delta = 0.0;
			double M2 = 0.0;
			double M2_b;
			double avg = 0.0, avg_a, avg_b;
			double min_wallclock_time = -1;
			double max_wallclock_time = -1;
			for (int j = 0; j < size; j++)
			{
				na = n;
				nb = recv_buf[j * 3 + 0];
				if (nb <= 0)
					continue;
				n = na + nb;
				avg_a = avg;
				avg_b = recv_buf[j * 3 + 1];
				delta = avg_b - avg_a;
				M2_b = recv_buf[j * 3 + 2];
				M2 += M2_b + delta * delta * na * nb / n;
				avg = (na * avg_a + nb * avg_b) / n;
			}
			double variance_wallclock_time = (n > 1) ? (M2 / (n - 1)) : -1.0;

			// Get total minimum and maximum wallclock time
			MPI_Reduce(&r->min_wallclock_time, &min_wallclock_time, 1, MPI_DOUBLE, MPI_MIN, 0, perf_regions.comm);
			MPI_Reduce(&r->max_wallclock_time, &max_wallclock_time, 1, MPI_DOUBLE, MPI_MAX, 0, perf_regions.comm);

			FILE *s = stdout;
			fprintf(s, "[PERF_REGIONS] section name: %s\n", r->region_name);
			fprintf(s, "[PERF_REGIONS] [%s].total_samples: %.0f\n", r->region_name, n);
			fprintf(s, "[PERF_REGIONS] [%s].min_wallclock_time: %.7e\n", r->region_name, min_wallclock_time);
			fprintf(s, "[PERF_REGIONS] [%s].max_wallclock_time: %.7e\n", r->region_name, max_wallclock_time);
			fprintf(s, "[PERF_REGIONS] [%s].mean_wallclock_time: %.7e\n", r->region_name, avg);
			fprintf(s, "[PERF_REGIONS] [%s].variance_wallclock_time: %.7e\n", r->region_name, variance_wallclock_time);

			free(recv_buf);
		}
		// Not root process? No mean calculation ot printing, just send the min and max wallclock time
		else
		{
			MPI_Reduce(&r->min_wallclock_time, NULL, 1, MPI_DOUBLE, MPI_MIN, 0, perf_regions.comm);
			MPI_Reduce(&r->max_wallclock_time, NULL, 1, MPI_DOUBLE, MPI_MAX, 0, perf_regions.comm);
		}
	}
	// Also print results of other performance counters but just for rank 0
	if (rank == 0)
	{
		fprintf(stdout, "Perf_regions, results for rank %d:\n", rank);
		perf_regions_output_human_readable_text();
	}
#  endif
#endif
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
		reduce_and_output_human_readable_text();
	}
	else 
	{
		/* output performance information on each region to console */
		perf_regions_output_human_readable_text();
	}
#else

/* output performance information on each region to console */
		perf_regions_output_human_readable_text();
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
#endif

#if PERF_COUNTERS_NESTED
	if (perf_regions.num_nested_performance_regions != 0)
	{
		fprintf(stderr, "num_nested_performance_regions != 0");
		exit(-1);
	}
#endif

	for (int i = 0; i < perf_regions.num_perf_counters; i++)
		free(perf_regions.perf_counter_names[i]);
}

