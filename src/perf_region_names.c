#include "perf_region_names.h"



static
const char* perf_region_names[PERF_REGIONS_MAX] =
{
	"[overhead a]",	// 0 - special performance region to measure overheads
	"[overhead b]",	// 1 - special performance region to measure overheads
	"[overhead c]",	// 2 - special performance region to measure overheads
	"foo",		// 3
	"bar",		// 4
	"xyz",		// 5
	"",			// 6
	"",			// 7
	"",			// 8
	"",			// 9
};



const char* get_perf_region_name(
		int i_id
)
{
	return perf_region_names[i_id];
}


