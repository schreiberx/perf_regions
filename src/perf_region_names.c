#include "perf_region_names.h"



static
const char* perf_region_names[PERF_REGIONS_MAX] =
{
	"[dummy]",	// 0
	"foo",		// 1
	"bar",		// 2
	"xyz",		// 3
	"",			// 4
	"",			// 5
	"",			// 6
	"",			// 7
	"",			// 8
	""			// 9
};



const char* get_perf_region_name(
		int i_id
)
{
	return perf_region_names[i_id];
}


