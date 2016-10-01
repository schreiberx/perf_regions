#include "perf_region_names.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static
const char** perf_region_names;



void perf_region_name_init()
{
	perf_region_names = malloc(sizeof(char*)*PERF_REGIONS_MAX);
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
		perf_region_names[i] = NULL;

	const char* perf_region_list_file = "";
	if (getenv("PERF_REGION_FILE") == NULL)
	{
		printf("PERF_REGION_FILE is not listed, searching for 'perf_region_list.txt'\n");
		perf_region_list_file = "perf_region_list.txt";
	}
	else
	{
		perf_region_list_file = getenv("PERF_REGION_FILE");
	}


	FILE *fp;
	char *buf_line = NULL;
	size_t buf_len = 0;
	ssize_t read;

	fp = fopen(perf_region_list_file, "r");
	if (fp != NULL)
	{
		int i = 0;
		while ((read = getline(&buf_line, &buf_len, fp)) != -1)
		{
			if (i >= PERF_REGIONS_MAX-3)
			{
				fprintf(stderr, "Number of performance regions exceeds PERF_REGIONS_MAX-3");
				exit(1);
			}

			// remove line break
			int len = strlen(buf_line);
			if (len > 0)
				if (buf_line[len-1] == '\n')
					buf_line[len-1] = '\0';

			perf_region_names[i] = strdup(buf_line);
			i++;
		}

		if (buf_line)
			free(buf_line);

		// fill the rest with nonsense
		for (; i < PERF_REGIONS_MAX-3; i++)
			perf_region_names[i] = strdup("");

		perf_region_names[PERF_REGIONS_OVERHEAD_TIMINGS] = strdup("RESERVED: OVERHEAD TIMINGS");
		perf_region_names[PERF_REGIONS_OVERHEAD_COUNTERS] = strdup("RESERVED: OVERHEAD COUNTERS");
		perf_region_names[PERF_REGIONS_OVERHEAD_TIMINGS_COUNTERS] = strdup("RESERVED: OVERHEAD TIMINGS COUNTERS");

		fclose(fp);
	}
	else
	{
		fprintf(stderr, "Unable to open file %s, using fallback enumeration", perf_region_list_file);

		char buffer[1024];
		for (int i = 0; i < PERF_REGIONS_MAX; i++)
		{
			sprintf(buffer, "REGION %i", i);
			perf_region_names[i] = strdup(buffer);
		}
	}
}



void perf_region_name_shutdown()
{
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
		free((char*)perf_region_names[i]);

	free(perf_region_names);
}



const char* get_perf_region_name(
		int i_id
)
{
	return perf_region_names[i_id];
}


