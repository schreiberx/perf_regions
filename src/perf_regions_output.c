#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "perf_regions.h"
#include "perf_regions_internal.h"
#include "perf_regions_output.h"

#define PRINT_PREFIX "[perf_regions_output.c] "

static int region_has_samples(const struct PerfRegion *r)
{
	double samples = r->region_enter_counter - r->region_skipped_counter;
	return samples > 0;
}

static double region_valid_samples(const struct PerfRegion *r)
{
	return r->region_enter_counter - r->region_skipped_counter;
}

struct RegionStats
{
	double valid_samples;
	double skipped_samples;
	double wallclock_time;
	double min_wallclock_time;
	double max_wallclock_time;
	double mean;
	double variance;
};

static struct RegionStats region_stats_from_perf_region(const struct PerfRegion *r)
{
	int has_samples = region_has_samples(r);
	struct RegionStats stats;

	stats.valid_samples = region_valid_samples(r);
	stats.skipped_samples = (double)r->region_skipped_counter;
	stats.wallclock_time = r->counter_wallclock_time;
	stats.min_wallclock_time = has_samples ? r->min_wallclock_time : 0.0;
	stats.max_wallclock_time = has_samples ? r->max_wallclock_time : 0.0;
	stats.mean = has_samples ? r->running_mean : 0.0;
	stats.variance = has_samples ? r->squared_dist_wallclock_time / stats.valid_samples : 0.0;

	return stats;
}

static void json_write_string(FILE *s, const char *str)
{
	if (str == NULL)
	{
		fputs("null", s);
		return;
	}

	fputc('"', s);
	for (const unsigned char *p = (const unsigned char *)str; *p != '\0'; p++)
	{
		switch (*p)
		{
			case '"':  fputs("\\\"", s); break;
			case '\\': fputs("\\\\", s); break;
			case '\b': fputs("\\b", s); break;
			case '\f': fputs("\\f", s); break;
			case '\n': fputs("\\n", s); break;
			case '\r': fputs("\\r", s); break;
			case '\t': fputs("\\t", s); break;
			default:
				if (*p < 0x20)
					fprintf(s, "\\u%04x", (unsigned int)*p);
				else
					fputc(*p, s);
				break;
		}
	}
	fputc('"', s);
}

static void print_human_header(FILE *s)
{
	fprintf(s, "[MULE] perf_regions: Performance counters profiling:\n");
	fprintf(s, "[MULE] perf_regions: ----------------------\n");
	fprintf(s, "[MULE] perf_regions: Section");

#if PERF_REGIONS_USE_PAPI
	for (int j = 0; j < perf_regions.num_perf_counters; j++)
		fprintf(s, "\t%s", perf_regions.perf_counter_names[j]);
#endif

#if PERF_COUNTERS_NESTED
	fprintf(s, "\tSPOILED");
#endif

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
}

static void print_human_region(
	FILE *s,
	const char *name,
	const long long *counters,
	int spoiled,
	const struct RegionStats *stats)
{
	fprintf(s, "[MULE] perf_regions: %s", name != NULL ? name : "(null)");

#if PERF_REGIONS_USE_PAPI
	for (int j = 0; j < perf_regions.num_perf_counters; j++)
	{
		long long value = (counters != NULL) ? counters[j] : 0;
		fprintf(s, "\t%.7e", (double)value);
	}
#else
	(void)counters;
#endif

#if PERF_COUNTERS_NESTED
	fprintf(s, "\t%i", spoiled);
#else
	(void)spoiled;
#endif

	if (perf_regions.use_wallclock_time)
	{
		fprintf(s, "\t%.7e", stats->wallclock_time);
		fprintf(s, "\t%.7e", stats->min_wallclock_time);
		fprintf(s, "\t%.7e", stats->max_wallclock_time);
		fprintf(s, "\t%.7e", stats->mean);
		fprintf(s, "\t%.7e", stats->variance);
	}

	fprintf(s, "\t%.0f", stats->valid_samples);
	fprintf(s, "\t%.0f", stats->skipped_samples);
	fprintf(s, "\n");
}

void perf_regions_output_human_readable_text(void)
{
	if (perf_regions.perf_regions_list == NULL)
		return;

	FILE *s = stdout;
	print_human_header(s);

#if PERF_DEBUG
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(perf_regions.perf_regions_list[i]);
		if (r->active != 0)
		{
			if (r->region_name != NULL)
				fprintf(stderr, "Still in region of %s\n", r->region_name);
			else
				fprintf(stderr, "Still in region NULL\n");

			exit(EXIT_FAILURE);
		}
	}
#endif

	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(perf_regions.perf_regions_list[i]);

		if (r->region_name == NULL)
			continue;

		struct RegionStats stats = region_stats_from_perf_region(r);

#if PERF_REGIONS_USE_PAPI
		const long long *counters = r->counter_values;
#else
		const long long *counters = NULL;
#endif

#if PERF_COUNTERS_NESTED
		int spoiled = r->spoiled;
#else
		int spoiled = 0;
#endif

		print_human_region(s, r->region_name, counters, spoiled, &stats);
	}
}

static void write_json_region(
	FILE *s,
	const char *name,
	const long long *counters,
	int spoiled,
	const struct RegionStats *stats)
{
	fprintf(s, "\t{\n\t\t\"region\": ");
	json_write_string(s, name);
	fprintf(s, ",\n");
	fprintf(s, "\t\t\"counter\": %.0f,\n", stats->valid_samples);
	fprintf(s, "\t\t\"skipped\": %.0f", stats->skipped_samples);

	if (perf_regions.use_wallclock_time)
	{
		fprintf(s, ",\n");
		fprintf(s, "\t\t\"wallclock_time\": %.7e,\n", stats->wallclock_time);
		fprintf(s, "\t\t\"min\": %.7e,\n", stats->min_wallclock_time);
		fprintf(s, "\t\t\"max\": %.7e,\n", stats->max_wallclock_time);
		fprintf(s, "\t\t\"mean\": %.7e,\n", stats->mean);
		fprintf(s, "\t\t\"variance\": %.7e", stats->variance);
	}

#if PERF_COUNTERS_NESTED
	fprintf(s, ",\n\t\t\"spoiled\": %d", spoiled);
#else
	(void)spoiled;
#endif

#if PERF_REGIONS_USE_PAPI
	if (perf_regions.num_perf_counters > 0)
	{
		fprintf(s, ",\n\t\t\"counters\": {\n");
		for (int j = 0; j < perf_regions.num_perf_counters; j++)
		{
			if (j > 0)
				fprintf(s, ",\n");
			fprintf(s, "\t\t\t");
			json_write_string(s, perf_regions.perf_counter_names[j]);
			fprintf(s, ": %.7e", counters != NULL ? (double)counters[j] : 0.0);
		}
		fprintf(s, "\n\t\t}");
	}
#else
	(void)counters;
#endif

	fprintf(s, "\n\t}");
}

void perf_regions_output_json_file(const char *filename)
{
	if (perf_regions.perf_regions_list == NULL)
		return;

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

		if (r->region_name == NULL)
			continue;

		if (!first_entry)
			fprintf(s, ",\n");
		first_entry = 0;

		struct RegionStats stats = region_stats_from_perf_region(r);

#if PERF_REGIONS_USE_PAPI
		const long long *counters = r->counter_values;
#else
		const long long *counters = NULL;
#endif

#if PERF_COUNTERS_NESTED
		int spoiled = r->spoiled;
#else
		int spoiled = 0;
#endif

		write_json_region(s, r->region_name, counters, spoiled, &stats);
	}

	fprintf(s, "\n]\n");

	if (filename != NULL)
		fclose(s);
}

#if USE_MPI

struct ReducedRegion
{
	int id;
	char *name;
	double valid_samples;
	double skipped_samples;
	double wallclock_time;
	double min_wallclock_time;
	double max_wallclock_time;
	double mean;
	double variance;
	int spoiled;
	long long *counters;
};

static struct RegionStats region_stats_from_reduced(const struct ReducedRegion *r)
{
	struct RegionStats stats;

	stats.valid_samples = r->valid_samples;
	stats.skipped_samples = r->skipped_samples;
	stats.wallclock_time = r->wallclock_time;
	stats.min_wallclock_time = r->min_wallclock_time;
	stats.max_wallclock_time = r->max_wallclock_time;
	stats.mean = r->mean;
	stats.variance = r->variance;

	return stats;
}

static void free_reduced_regions(struct ReducedRegion *regions, int count)
{
	if (regions == NULL)
		return;

	for (int i = 0; i < count; i++)
	{
		free(regions[i].name);
		free(regions[i].counters);
	}
	free(regions);
}

/*
 * Reduce all regions over the MPI communicator.
 *
 * Every rank executes the same collectives for every region ID.  This is
 * important for conditionally executed regions: a rank that never entered a
 * region still participates with zero samples.
 */
static void reduce_all_regions(struct ReducedRegion **out_regions, int *out_count)
{
	*out_regions = NULL;
	*out_count = 0;

	int rank, size;
	MPI_Comm_rank(perf_regions.comm, &rank);
	MPI_Comm_size(perf_regions.comm, &size);

	struct ReducedRegion *regions = NULL;
	if (rank == 0)
	{
		regions = (struct ReducedRegion *)calloc(PERF_REGIONS_MAX, sizeof(struct ReducedRegion));
		if (regions == NULL)
		{
			fprintf(stderr, PRINT_PREFIX"Failed to allocate reduced regions\n");
			exit(EXIT_FAILURE);
		}
	}

	int count = 0;

	for (int id = 0; id < PERF_REGIONS_MAX; id++)
	{
		struct PerfRegion *r = &(perf_regions.perf_regions_list[id]);
		int local_present = (r->region_name != NULL);
		int local_name_len = local_present ? (int)strlen(r->region_name) + 1 : 0;

		int max_name_len = 0;
		MPI_Allreduce(&local_name_len, &max_name_len, 1, MPI_INT, MPI_MAX, perf_regions.comm);

		if (max_name_len == 0)
			continue;

		int owner = local_present ? rank : size;
		int first_owner = size;
		MPI_Allreduce(&owner, &first_owner, 1, MPI_INT, MPI_MIN, perf_regions.comm);

		char *name_buf = (char *)malloc((size_t)max_name_len);
		if (name_buf == NULL)
		{
			fprintf(stderr, PRINT_PREFIX"Failed to allocate region name buffer\n");
			exit(EXIT_FAILURE);
		}

		memset(name_buf, 0, (size_t)max_name_len);
		if (rank == first_owner)
			memcpy(name_buf, r->region_name, (size_t)local_name_len);

		MPI_Bcast(name_buf, max_name_len, MPI_CHAR, first_owner, perf_regions.comm);

		double valid_samples = local_present ? region_valid_samples(r) : 0.0;
		double mean = local_present ? r->running_mean : 0.0;
		double m2 = local_present ? r->squared_dist_wallclock_time : 0.0;

		int has_samples = valid_samples > 0;
		double min_wallclock_time = has_samples ? r->min_wallclock_time : DBL_MAX;
		double max_wallclock_time = has_samples ? r->max_wallclock_time : -DBL_MAX;
		double skipped_samples = local_present ? (double)r->region_skipped_counter : 0.0;

#if PERF_COUNTERS_NESTED
		double spoiled = (local_present && r->spoiled) ? 1.0 : 0.0;
#else
		double spoiled = 0.0;
#endif

		// n, mean, M2, min, max, skipped, spoiled
		double send_stats[7] = {
			valid_samples, mean, m2, min_wallclock_time,
			max_wallclock_time, skipped_samples, spoiled
		};
		double *recv_stats = NULL;
		if (rank == 0)
		{
			recv_stats = (double *)malloc((size_t)size * 7 * sizeof(double));
			if (recv_stats == NULL)
			{
				fprintf(stderr, PRINT_PREFIX"Failed to allocate receive buffer\n");
				exit(EXIT_FAILURE);
			}
		}

		MPI_Gather(send_stats, 7, MPI_DOUBLE, recv_stats, 7, MPI_DOUBLE,
			0, perf_regions.comm);

		long long *recv_counters = NULL;
#if PERF_REGIONS_USE_PAPI
		int local_nc = perf_regions.num_perf_counters;
		int max_nc = 0;
		MPI_Allreduce(&local_nc, &max_nc, 1, MPI_INT, MPI_MAX, perf_regions.comm);

		if (max_nc > 0)
		{
			long long *send_counters = (long long *)calloc((size_t)max_nc, sizeof(long long));
			if (send_counters == NULL)
			{
				fprintf(stderr, PRINT_PREFIX"Failed to allocate counter send buffer\n");
				exit(EXIT_FAILURE);
			}

			if (local_present)
			{
				int ncopy = local_nc < max_nc ? local_nc : max_nc;
				for (int j = 0; j < ncopy; j++)
					send_counters[j] = r->counter_values[j];
			}

			if (rank == 0)
			{
				recv_counters = (long long *)calloc((size_t)max_nc, sizeof(long long));
				if (recv_counters == NULL)
				{
					fprintf(stderr, PRINT_PREFIX"Failed to allocate counter receive buffer\n");
					exit(EXIT_FAILURE);
				}
			}

			MPI_Reduce(send_counters, recv_counters, max_nc, MPI_LONG_LONG_INT,
				MPI_SUM, 0, perf_regions.comm);
			free(send_counters);
		}
#endif

		if (rank == 0)
		{
			double n = 0.0;
			double avg = 0.0;
			double M2 = 0.0;
			double min_all = DBL_MAX;
			double max_all = -DBL_MAX;
			double skipped_all = 0.0;
			int spoiled_all = 0;

			for (int j = 0; j < size; j++)
			{
				double nb = recv_stats[j * 7 + 0];
				double skipped = recv_stats[j * 7 + 5];
				int spoiled_rank = recv_stats[j * 7 + 6] != 0.0;

				skipped_all += skipped;
				spoiled_all |= spoiled_rank;

				if (nb <= 0.0)
					continue;

				double na = n;
				double avg_b = recv_stats[j * 7 + 1];
				double M2_b = recv_stats[j * 7 + 2];
				double delta = avg_b - avg;

				n = na + nb;
				M2 += M2_b + delta * delta * na * nb / n;
				avg = (na * avg + nb * avg_b) / n;

				double min_rank = recv_stats[j * 7 + 3];
				double max_rank = recv_stats[j * 7 + 4];
				if (min_rank < min_all)
					min_all = min_rank;
				if (max_rank > max_all)
					max_all = max_rank;
			}

			struct ReducedRegion *rr = &regions[count];
			rr->id = id;
			rr->name = name_buf;
			name_buf = NULL;
			rr->valid_samples = n;
			rr->skipped_samples = skipped_all;
			rr->mean = (n > 0.0) ? avg : 0.0;
			rr->variance = (n > 0.0) ? M2 / n : 0.0;
			rr->min_wallclock_time = (n > 0.0) ? min_all : 0.0;
			rr->max_wallclock_time = (n > 0.0) ? max_all : 0.0;
			rr->wallclock_time = (n > 0.0) ? avg * n : 0.0;
			rr->spoiled = spoiled_all;
			rr->counters = recv_counters;
			count++;
		}

		free(name_buf);
		free(recv_stats);
	}

	*out_regions = regions;
	*out_count = count;
}

void reduce_and_output_human_readable_text(void)
{
	if (perf_regions.perf_regions_list == NULL)
		return;

	int rank;
	MPI_Comm_rank(perf_regions.comm, &rank);

	struct ReducedRegion *regions = NULL;
	int count = 0;
	reduce_all_regions(&regions, &count);

	if (rank == 0)
	{
		FILE *s = stdout;
		print_human_header(s);

		for (int i = 0; i < count; i++)
		{
			struct ReducedRegion *r = &regions[i];
			struct RegionStats stats = region_stats_from_reduced(r);
#if PERF_REGIONS_USE_PAPI
			const long long *counters = r->counters;
#else
			const long long *counters = NULL;
#endif
			print_human_region(s, r->name, counters, r->spoiled, &stats);
		}
	}

	free_reduced_regions(regions, count);
}

void reduce_and_output_json_file(const char *filename)
{
	if (perf_regions.perf_regions_list == NULL)
		return;

	int rank;
	MPI_Comm_rank(perf_regions.comm, &rank);

	struct ReducedRegion *regions = NULL;
	int count = 0;
	reduce_all_regions(&regions, &count);

	if (rank == 0)
	{
		FILE *s;
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
			for (int i = 0; i < count; i++)
			{
				struct ReducedRegion *r = &regions[i];
				struct RegionStats stats = region_stats_from_reduced(r);
				if (i > 0)
					fprintf(s, ",\n");

#if PERF_REGIONS_USE_PAPI
				const long long *counters = r->counters;
#else
				const long long *counters = NULL;
#endif
				write_json_region(s, r->name, counters, r->spoiled, &stats);
			}
			fprintf(s, "\n]\n");

			if (filename != NULL)
				fclose(s);
		}
	}

	free_reduced_regions(regions, count);
}

#else

void reduce_and_output_human_readable_text(void)
{
}

void reduce_and_output_json_file(const char *filename)
{
	(void)filename;
}

#endif
