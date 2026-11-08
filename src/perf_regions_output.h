#ifndef PERF_REGIONS_OUTPUT_H
#define PERF_REGIONS_OUTPUT_H

void perf_regions_output_human_readable_text(void);
void perf_regions_output_json_file(const char *filename);
void reduce_and_output_human_readable_text(void);
void reduce_and_output_json_file(const char *filename);

#endif
