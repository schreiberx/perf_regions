#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <float.h>
#include <sys/time.h>
#include <string.h>

//TODO ifdef
#include <mpi.h>


#include "perf_regions.h"
#include "papi_counters.h"

#include "perf_regions_defines.h"
//#include "perf_regions_names.h"


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
	double variance_wallclock_time;
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
		r->max_wallclock_time = 0;
		r->min_wallclock_time = DBL_MAX;
		r->variance_wallclock_time = 0;
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

	papi_counters_init(perf_regions.perf_counter_names, perf_regions.num_perf_counters, perf_regions.verbosity);

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



/**
 * start measuring the performance for region given by the ID.
 */
void perf_region_start(
	int i_region_id,	///< id of region
	const char* i_region_name	///< region name (associated with ID)
)
{
	struct PerfRegion *r = &(perf_regions.perf_regions_list[i_region_id]);

	// denominator to normalize values
	r->region_enter_counter++;

	// Initialize only once
	// There's a small overhead, but we hope that this is negligible since we also do this before starting performance counting
	if (r->region_name == 0)
		r->region_name = strdup(i_region_name);

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
#if PERF_COUNTERS_NESTED

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

#else
		papi_counters_start();
#endif
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
#if PERF_COUNTERS_NESTED

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

#else

		long long counter_values_stop[PERF_COUNTERS_MAX];
		papi_counters_stop(counter_values_stop);

		for (int j = 0; j < perf_regions.num_perf_counters; j++)
			r->counter_values[j] += counter_values_stop[j];

#endif
	}
#endif

	if (perf_regions.use_wallclock_time)
	{
	#ifdef PERF_TIMING_POSIX
		double last_wallclock_time_measurement = posix_clock() - r->region_enter_time_posix; 
	#else
		struct timeval tm2;
		gettimeofday(&tm2, NULL);
		double last_wallclock_time_measurement = ((double)time_val.tv_sec - (double)r->start_time_value.tv_sec)
					+ ((double)time_val.tv_usec - (double)r->start_time_value.tv_usec)*0.000001;					
	#endif

		r->counter_wallclock_time += last_wallclock_time_measurement;
		r->max_wallclock_time = r->max_wallclock_time > last_wallclock_time_measurement?r->max_wallclock_time:last_wallclock_time_measurement;
		r->min_wallclock_time = r->min_wallclock_time < last_wallclock_time_measurement?r->min_wallclock_time:last_wallclock_time_measurement;
		// Variance with Welford's online algorithm
		double delta = last_wallclock_time_measurement - r->running_mean;
		r->running_mean += delta / r->region_enter_counter;
		r->variance_wallclock_time += delta * (last_wallclock_time_measurement - r->running_mean);

	}
}



void perf_regions_output_human_readable_text()
{

#if PERF_REGIONS_USE_PAPI
	FILE *s = stdout;

	fprintf(s, "Performance counters profiling:\n");
	fprintf(s, "----------------------\n");
	fprintf(s, "Section");
	for (int j = 0; j < perf_regions.num_perf_counters; j++)
		fprintf(s, "\t%s", perf_regions.perf_counter_names[j]);
#if PERF_COUNTERS_NESTED
	fprintf(s, "\tSPOILED");
#endif
//TODO format
	if (perf_regions.use_wallclock_time) {
		fprintf(s, "\tWALLCLOCKTIME");
		fprintf(s, "\tMIN");
		fprintf(s, "\t\t\tMAX");
		fprintf(s, "\t\t\tMEAN");
		fprintf(s, "\t\t\tVAR");
	}
	fprintf(s, "\tCOUNTER");
	fprintf(s, "\n");

#if PERF_DEBUG
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
#endif

	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(perf_regions.perf_regions_list[i]);

		if (r->region_name == 0)
			continue;

		fprintf(s, "%s", r->region_name);

		for (int j = 0; j < perf_regions.num_perf_counters; j++)
		{
			long long counter_value = r->counter_values[j];

			// DON'T NORMALIZE the performance value
			//counter_value /= r->region_enter_counter;

			double param_value = counter_value;

			fprintf(s, "\t%.7e", param_value);
		}
#if PERF_COUNTERS_NESTED
		fprintf(s, "\t\t%i\t", r->spoiled);
#endif
		if (perf_regions.use_wallclock_time) {
			fprintf(s, "\t%.7e", r->counter_wallclock_time);
			fprintf(s, "\t%.7e", r->min_wallclock_time);
			fprintf(s, "\t%.7e", r->max_wallclock_time);
			fprintf(s, "\t%.7e", r->running_mean);
			fprintf(s, "\t%.7e", r->variance_wallclock_time / r->region_enter_counter);
		}

		fprintf(s, "\t\t\t%.0f", r->region_enter_counter);

		fprintf(s, "\n");
	}
#endif

}


void perf_regions_output_csv_file()
{

}

// TODO change in prinstumentation, header, sth for fortran
void perf_regions_reduce(int communicator) {
	MPI_Comm comm = MPI_Comm_f2c((MPI_Fint)communicator);

	int rank, size;
	MPI_Comm_rank(comm, &rank);
	MPI_Comm_size(comm, &size);

	#if PERF_REGIONS_USE_PAPI
	
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(perf_regions.perf_regions_list[i]);

		if (r->region_name == 0)
			continue;

		for (int j = 0; j < perf_regions.num_perf_counters; j++)
		{
			long long counter_value = r->counter_values[j];

			double param_value = counter_value;
			// TODO what to do with these?
			//fprintf(s, "\t%.7e", param_value);
		}
		if (perf_regions.use_wallclock_time) {
			double send_data[4] = {
			r->counter_wallclock_time,
			r->min_wallclock_time,
			r->max_wallclock_time,
			r->variance_wallclock_time / r->region_enter_counter
			};
			double recv_data[3][4]= {0};
		// TODO do I want enter? (It is int)

			const int ROOT_RANK = 0;
			const MPI_Datatype DATATYPE = MPI_DOUBLE;
			const int DATA_COUNT = 4;

			// MIN, MAX, MEAN, VAR - min, max, sum, ?

			const MPI_Op REDUCTION_OPS[3] = {MPI_MIN, MPI_MAX, MPI_SUM};
    		const char* OP_NAMES[3] = {"MIN", "MAX", "AVG"};

			if(rank==ROOT_RANK) {
				FILE *s = stdout;
				fprintf(s, "\t\tWALLCLOCKTIME\tMIN\t\t\tMAX\t\t\tVAR\n");
			}


			for (int o = 0; o < 3; o++) {
			MPI_Reduce(send_data, recv_data[o], DATA_COUNT, 
					DATATYPE, REDUCTION_OPS[o], ROOT_RANK, comm);

				if(rank==ROOT_RANK) {
					// make sum to avg
					if(o==2) {
						double n = 1.0*size;
						recv_data[o][0] /= n;
						recv_data[o][1] /= n;
						recv_data[o][2] /= n;
						recv_data[o][3] /= n;
					}
					FILE *s = stdout;
					fprintf(s, "%s(rank)\t%.7e\t%.7e\t%.7e\t%.7e\n", OP_NAMES[o], recv_data[o][0], recv_data[o][1], recv_data[o][2], recv_data[o][3]);
				}

			}
			
		}
		
	}
	if (rank==0)
		perf_regions_output_human_readable_text();
	perf_regions_shutdown();
#endif	

}

void perf_regions_shutdown() {
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

	papi_counters_finalize();

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

/**
 * DECONSTRUCTOR
 */
void perf_regions_finalize()
{
	/*
	 * output performance information on each region to console
	 */
	perf_regions_output_human_readable_text();

	/*
	 * Output .csv file
	 */
	perf_regions_output_csv_file();

	perf_regions_shutdown();
	
}
