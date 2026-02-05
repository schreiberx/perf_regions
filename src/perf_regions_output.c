#include <stdio.h>
#include <stdlib.h>
#include <math.h> 

#include "perf_regions.h"
#include "perf_regions_output.h"

#define PRINT_PREFIX "[perf_regions_output.c] "

void perf_regions_output_human_readable_text()
{

	FILE *s = stdout;

	fprintf(s, "[MULE] perf_regions: Performance counters profiling:\n");
	fprintf(s, "[MULE] perf_regions: ----------------------\n");
	fprintf(s, "[MULE] perf_regions: Section");

#	if PERF_REGIONS_USE_PAPI
	for (int j = 0; j < perf_regions.num_perf_counters; j++)
		fprintf(s, "\t%s", perf_regions.perf_counter_names[j]);
#	endif

#   if PERF_COUNTERS_NESTED
	fprintf(s, "\tSPOILED");
#   endif
	if (perf_regions.use_wallclock_time)
	{
		fprintf(s, "\tWALLCLOCKTIME");
		fprintf(s, "\tMIN");
		fprintf(s, "\tMAX");
		fprintf(s, "\tMEAN");
		fprintf(s, "\tVAR");
	}
	fprintf(s, "\tCOUNTER");
	fprintf(s, "\tSKIPPED");
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
			
			// Subtract skipped measurements from region_enter_counter for variance
			double valid_samples = r->region_enter_counter - r->region_skipped_counter;
			if (valid_samples > 0)
				fprintf(s, "\t%.7e", r->squared_dist_wallclock_time / valid_samples);
			else
				fprintf(s, "\t%.7e", 0.0);
		}

		// Output counter (subtract skipped?)
		// User: "subtract this from the region_enter_counter"
        // I will output the effective valid counter.
		fprintf(s, "\t%.0f", r->region_enter_counter - r->region_skipped_counter);
		fprintf(s, "\t%lld", r->region_skipped_counter);

		fprintf(s, "\n");
	}
}


void perf_regions_output_csv_file()
{

}

void perf_regions_output_json_file(const char* filename)
{
	FILE *s;
	if (filename == NULL)
		s = stdout;
	else
		s = fopen(filename, "w");

	if (s == NULL)
	{
		fprintf(stderr, "PERF_REGIONS: Failed to open JSON output file '%s'\n", filename);
		return;
	}

	fprintf(s, "[\n");
	int first_entry = 1;

	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(perf_regions.perf_regions_list[i]);

		if (r->region_name == 0)
			continue;

		if (!first_entry)
			fprintf(s, ",\n");
		first_entry = 0;

		fprintf(s, "\t{\n");
		fprintf(s, "\t\t\"region\": \"%s\",\n", r->region_name);
		
		double valid_samples = r->region_enter_counter - r->region_skipped_counter;
		fprintf(s, "\t\t\"counter\": %.0f,\n", valid_samples);
		fprintf(s, "\t\t\"skipped\": %lld", r->region_skipped_counter);

		if (perf_regions.use_wallclock_time) {
			fprintf(s, ",\n");
			fprintf(s, "\t\t\"wallclock_time\": %.7e,\n", r->counter_wallclock_time);
			fprintf(s, "\t\t\"min\": %.7e,\n", r->min_wallclock_time);
			fprintf(s, "\t\t\"max\": %.7e,\n", r->max_wallclock_time);
			fprintf(s, "\t\t\"mean\": %.7e,\n", r->running_mean);
			
			double variance = 0.0;
			if (valid_samples > 0)
				variance = r->squared_dist_wallclock_time / valid_samples;
			fprintf(s, "\t\t\"variance\": %.7e", variance);
		}
#if PERF_COUNTERS_NESTED
		fprintf(s, ",\n\t\t\"spoiled\": %d", r->spoiled);
#endif

#if PERF_REGIONS_USE_PAPI
		if (perf_regions.num_perf_counters > 0)
		{
			fprintf(s, ",\n\t\t\"counters\": {\n");
			for (int j = 0; j < perf_regions.num_perf_counters; j++)
			{
				if (j > 0) fprintf(s, ",\n");
				fprintf(s, "\t\t\t\"%s\": %.7e", perf_regions.perf_counter_names[j], (double)r->counter_values[j]);
			}
			fprintf(s, "\n\t\t}");
		}
#endif
		fprintf(s, "\n\t}");
	}
	fprintf(s, "\n]\n");

	if (filename != NULL)
		fclose(s);
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
		// Adjust region_enter_counter to reflect valid samples
		double valid_samples = r->region_enter_counter - r->region_skipped_counter;
		double send_buf[3] = {valid_samples, r->running_mean, r->squared_dist_wallclock_time};
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

void reduce_and_output_json_file(const char* filename)
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

	FILE *s = NULL;
	if (rank == 0)
	{
		if (filename == NULL)
			s = stdout;
		else
			s = fopen(filename, "w");
		
		if (s == NULL)
		{
			fprintf(stderr, "PERF_REGIONS: Failed to open JSON output file '%s'\n", filename);
		}
		else
		{
			fprintf(s, "[\n");
		}
	}


	int first_entry = 1;
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
		double send_buf[3] = {r->region_enter_counter - r->region_skipped_counter, r->running_mean, r->squared_dist_wallclock_time};
		MPI_Gather(send_buf, 3, MPI_DOUBLE, recv_buf, 3, MPI_DOUBLE, 0, perf_regions.comm);

		// Calculate mean and variance of wallclock time across all processes
		// use the Welford/Chan parallel algorithm so it's numerically stable
		if (rank == 0 && s != NULL)
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
			
			// Output JSON
			if (!first_entry)
				fprintf(s, ",\n");
			first_entry = 0;

			fprintf(s, "\t{\n");
			fprintf(s, "\t\t\"region\": \"%s\",\n", r->region_name);
			fprintf(s, "\t\t\"total_samples\": %.0f,\n", n);
			fprintf(s, "\t\t\"min_wallclock_time\": %.7e,\n", min_wallclock_time);
			fprintf(s, "\t\t\"max_wallclock_time\": %.7e,\n", max_wallclock_time);
			fprintf(s, "\t\t\"mean_wallclock_time\": %.7e,\n", avg);
			fprintf(s, "\t\t\"variance_wallclock_time\": %.7e", variance_wallclock_time);
			fprintf(s, "\n\t}");

			free(recv_buf);
		}
		// Not root process? No mean calculation ot printing, just send the min and max wallclock time
		else
		{
			MPI_Reduce(&r->min_wallclock_time, NULL, 1, MPI_DOUBLE, MPI_MIN, 0, perf_regions.comm);
			MPI_Reduce(&r->max_wallclock_time, NULL, 1, MPI_DOUBLE, MPI_MAX, 0, perf_regions.comm);
		}
	}
	
	if (rank == 0 && s != NULL)
	{
		fprintf(s, "\n]\n");
		if (filename != NULL)
			fclose(s);
	}
#  endif
#endif
}
