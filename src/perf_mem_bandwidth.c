#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "perf_mem_bandwidth.h"

#if PERF_REGIONS_USE_PAPI

#include <papi.h>
#include <unistd.h>

#define PRINT_PREFIX "[perf_mem_bandwidth.c] "

struct MemEvent
{
	char *name;
	int component_index;
	int uncore;
};

struct MemInventory
{
	struct MemEvent *items;
	int count;
	int capacity;
};

struct MemSelection
{
	char *name;
	double bytes_per_count;
};

struct MemSelectionList
{
	struct MemSelection *items;
	int count;
	int capacity;
};

static void *mem_realloc(void *ptr, size_t size)
{
	void *result = realloc(ptr, size);
	if (result == NULL)
	{
		fprintf(stderr, PRINT_PREFIX"Out of memory\n");
		exit(EXIT_FAILURE);
	}
	return result;
}

static char *mem_strdup(const char *s)
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

static void mem_trim(char *s)
{
	if (s == NULL)
		return;

	char *start = s;
	while (*start != '\0' && isspace((unsigned char)*start))
		start++;

	if (start != s)
		memmove(s, start, strlen(start) + 1);

	size_t len = strlen(s);
	while (len > 0 && isspace((unsigned char)s[len - 1]))
		s[--len] = '\0';
}

static const char *event_without_pmu(const char *name)
{
	const char *sep = strstr(name, "::");
	return (sep != NULL) ? sep + 2 : name;
}

static void event_pmu(const char *name, char *out, size_t out_size)
{
	const char *sep = strstr(name, "::");
	if (sep == NULL)
	{
		out[0] = '\0';
		return;
	}

	size_t len = (size_t)(sep - name);
	if (len >= out_size)
		len = out_size - 1;
	memcpy(out, name, len);
	out[len] = '\0';
}

static void event_base(const char *name, char *out, size_t out_size)
{
	const char *rest = event_without_pmu(name);
	size_t len = strcspn(rest, ":.");

	if (len >= out_size)
		len = out_size - 1;
	memcpy(out, rest, len);
	out[len] = '\0';
}

static void event_umask(const char *name, char *out, size_t out_size)
{
	const char *rest = event_without_pmu(name);
	const char *p = rest + strcspn(rest, ":.");
	if (*p == '\0')
	{
		out[0] = '\0';
		return;
	}

	p++;
	size_t len = strcspn(p, ":.");
	if (len >= out_size)
		len = out_size - 1;
	memcpy(out, p, len);
	out[len] = '\0';
}

static int contains(const char *haystack, const char *needle)
{
	return haystack != NULL && strstr(haystack, needle) != NULL;
}

static int is_uncore_component(int component_index)
{
	const PAPI_component_info_t *component = PAPI_get_component_info(component_index);
	if (component == NULL)
		return 0;

	return strstr(component->name, "uncore") != NULL;
}

static int inventory_add(
	struct MemInventory *inventory,
	const char *name,
	int component_index
)
{
	for (int i = 0; i < inventory->count; i++)
	{
		if (strcmp(inventory->items[i].name, name) == 0)
			return 0;
	}

	if (inventory->count >= inventory->capacity)
	{
		int new_capacity = inventory->capacity == 0 ? 256 : inventory->capacity * 2;
		inventory->items = (struct MemEvent *)mem_realloc(
			inventory->items,
			(size_t)new_capacity * sizeof(struct MemEvent));
		inventory->capacity = new_capacity;
	}

	struct MemEvent *event = &inventory->items[inventory->count++];
	event->name = mem_strdup(name);
	event->component_index = component_index;
	event->uncore = is_uncore_component(component_index);
	return 1;
}

static void inventory_free(struct MemInventory *inventory)
{
	for (int i = 0; i < inventory->count; i++)
		free(inventory->items[i].name);
	free(inventory->items);
	inventory->items = NULL;
	inventory->count = 0;
	inventory->capacity = 0;
}

static void inventory_enumerate_component(
	struct MemInventory *inventory,
	int component_index
)
{
	int event_code = 0 | PAPI_NATIVE_MASK;
	int retval = PAPI_enum_cmp_event(&event_code, PAPI_ENUM_FIRST, component_index);
	if (retval != PAPI_OK)
		return;

	do
	{
		PAPI_event_info_t info;
		if (PAPI_get_event_info(event_code, &info) == PAPI_OK && info.symbol[0] != '\0')
		{
			inventory_add(inventory, info.symbol, component_index);

			int umask_code = event_code;
			if (PAPI_enum_cmp_event(&umask_code, PAPI_NTV_ENUM_UMASKS, component_index) == PAPI_OK)
			{
				do
				{
					PAPI_event_info_t umask_info;
					if (PAPI_get_event_info(umask_code, &umask_info) == PAPI_OK &&
					    umask_info.symbol[0] != '\0')
						inventory_add(inventory, umask_info.symbol, component_index);
				}
				while (PAPI_enum_cmp_event(&umask_code, PAPI_NTV_ENUM_UMASKS, component_index) == PAPI_OK);
			}
		}
	}
	while (PAPI_enum_cmp_event(&event_code, PAPI_ENUM_EVENTS, component_index) == PAPI_OK);
}

static void inventory_build(struct MemInventory *inventory)
{
	int num_components = PAPI_num_components();
	for (int cidx = 0; cidx < num_components; cidx++)
	{
		const PAPI_component_info_t *component = PAPI_get_component_info(cidx);
		if (component == NULL)
			continue;

		if (component->disabled != 0 && component->disabled != PAPI_EDELAY_INIT)
			continue;

		inventory_enumerate_component(inventory, cidx);
	}
}

static void selection_add_name(
	struct MemSelectionList *selection,
	const char *name,
	double bytes_per_count
)
{
	for (int i = 0; i < selection->count; i++)
	{
		if (strcmp(selection->items[i].name, name) == 0)
			return;
	}

	if (selection->count >= selection->capacity)
	{
		int new_capacity = selection->capacity == 0 ? 16 : selection->capacity * 2;
		selection->items = (struct MemSelection *)mem_realloc(
			selection->items,
			(size_t)new_capacity * sizeof(struct MemSelection));
		selection->capacity = new_capacity;
	}

	struct MemSelection *event = &selection->items[selection->count++];
	event->name = mem_strdup(name);
	event->bytes_per_count = bytes_per_count;
}

static void selection_add_event(
	struct MemSelectionList *selection,
	const struct MemEvent *event,
	double bytes_per_count,
	const int *cpus,
	int num_cpus
)
{
	if (!event->uncore || strstr(event->name, ":cpu=") != NULL || num_cpus <= 0)
	{
		selection_add_name(selection, event->name, bytes_per_count);
		return;
	}

	for (int i = 0; i < num_cpus; i++)
	{
		char name[1024];
		int written = snprintf(name, sizeof(name), "%s:cpu=%d", event->name, cpus[i]);
		if (written > 0 && (size_t)written < sizeof(name))
			selection_add_name(selection, name, bytes_per_count);
	}
}

static void selection_free_names(struct MemSelectionList *selection)
{
	for (int i = 0; i < selection->count; i++)
		free(selection->items[i].name);
	free(selection->items);
	selection->items = NULL;
	selection->count = 0;
	selection->capacity = 0;
}

static int add_matching_intel_imc(
	struct MemSelectionList *selection,
	const struct MemInventory *inventory,
	const int *cpus,
	int num_cpus,
	const char *required_umask
)
{
	int before = selection->count;

	for (int i = 0; i < inventory->count; i++)
	{
		const struct MemEvent *event = &inventory->items[i];
		char pmu[128];
		char base[256];
		char umask[128];

		event_pmu(event->name, pmu, sizeof(pmu));
		event_base(event->name, base, sizeof(base));
		event_umask(event->name, umask, sizeof(umask));

		if (!contains(pmu, "unc_imc"))
			continue;

		if (strncmp(base, "UNC_M_CAS_COUNT", 15) != 0)
			continue;

		if (required_umask == NULL)
		{
			if (umask[0] != '\0')
				continue;
		}
		else if (strcmp(umask, required_umask) != 0)
		{
			continue;
		}

		selection_add_event(selection, event, 64.0, cpus, num_cpus);
	}

	return selection->count - before;
}

static int select_intel_imc(
	struct MemSelectionList *selection,
	const struct MemInventory *inventory,
	const int *cpus,
	int num_cpus
)
{
	int added = add_matching_intel_imc(selection, inventory, cpus, num_cpus, "RD");
	added += add_matching_intel_imc(selection, inventory, cpus, num_cpus, "WR");
	if (added > 0)
		return added;

	added = add_matching_intel_imc(selection, inventory, cpus, num_cpus, "ALL");
	if (added > 0)
		return added;

	return add_matching_intel_imc(selection, inventory, cpus, num_cpus, NULL);
}

static int select_intel_qmc(
	struct MemSelectionList *selection,
	const struct MemInventory *inventory,
	const int *cpus,
	int num_cpus
)
{
	int before = selection->count;

	for (int i = 0; i < inventory->count; i++)
	{
		const struct MemEvent *event = &inventory->items[i];
		char base[256];
		char umask[128];

		event_base(event->name, base, sizeof(base));
		event_umask(event->name, umask, sizeof(umask));

		int is_read_any =
			(strcmp(base, "UNC_QMC_NORMAL_READS") == 0 && strcmp(umask, "ANY") == 0) ||
			strcmp(base, "UNC_QMC_NORMAL_READS_ANY") == 0;
		int is_write_full_any =
			(strcmp(base, "UNC_QMC_WRITES") == 0 && strcmp(umask, "FULL_ANY") == 0) ||
			strcmp(base, "UNC_QMC_WRITES_FULL_ANY") == 0;

		if (is_read_any || is_write_full_any)
			selection_add_event(selection, event, 64.0, cpus, num_cpus);
	}

	return selection->count - before;
}

static int select_intel_dram(
	struct MemSelectionList *selection,
	const struct MemInventory *inventory,
	const int *cpus,
	int num_cpus
)
{
	static const char *const event_names[] = {
		"UNC_DRAM_READ_CAS",
		"UNC_DRAM_WRITE_CAS",
		"UNC_FVC_EV0_BBOX_CMDS_READS",
		"UNC_DRAM_CMD_CAS_WR_OPN",
		"UNC_DRAM_MISC_CAS_WR_CLS",
	};
	int before = selection->count;

	for (int i = 0; i < inventory->count; i++)
	{
		const struct MemEvent *event = &inventory->items[i];
		char base[256];
		char umask[128];

		event_base(event->name, base, sizeof(base));
		event_umask(event->name, umask, sizeof(umask));

		if (umask[0] != '\0' && strcmp(umask, "ALL") != 0)
			continue;

		for (size_t j = 0; j < sizeof(event_names) / sizeof(event_names[0]); j++)
		{
			if (strcmp(base, event_names[j]) == 0)
			{
				selection_add_event(selection, event, 64.0, cpus, num_cpus);
				break;
			}
		}
	}

	return selection->count - before;
}

static int select_intel_client(
	struct MemSelectionList *selection,
	const struct MemInventory *inventory,
	const int *cpus,
	int num_cpus
)
{
	static const char *const event_names[] = {
		"DRAM_READS",
		"DRAM_WRITES",
		"UNC_M_DRAM_READS",
		"UNC_M_DRAM_WRITES",
	};
	int before = selection->count;

	for (int i = 0; i < inventory->count; i++)
	{
		const struct MemEvent *event = &inventory->items[i];
		char base[256];
		char umask[128];

		event_base(event->name, base, sizeof(base));
		event_umask(event->name, umask, sizeof(umask));

		if (umask[0] != '\0' && strcmp(umask, "ALL") != 0)
			continue;

		for (size_t j = 0; j < sizeof(event_names) / sizeof(event_names[0]); j++)
		{
			if (strcmp(base, event_names[j]) == 0)
			{
				selection_add_event(selection, event, 64.0, cpus, num_cpus);
				break;
			}
		}
	}

	return selection->count - before;
}

static int select_amd_umc(
	struct MemSelectionList *selection,
	const struct MemInventory *inventory,
	const int *cpus,
	int num_cpus
)
{
	int before = selection->count;

	for (int i = 0; i < inventory->count; i++)
	{
		const struct MemEvent *event = &inventory->items[i];
		char base[256];
		char umask[128];

		event_base(event->name, base, sizeof(base));
		event_umask(event->name, umask, sizeof(umask));

		if (!contains(base, "CAS_CMD"))
			continue;

		if (contains(base, "RD") || contains(base, "WR") ||
		    strcmp(umask, "RD") == 0 || strcmp(umask, "WR") == 0)
			selection_add_event(selection, event, 64.0, cpus, num_cpus);
	}

	return selection->count - before;
}

static int select_amd_df(
	struct MemSelectionList *selection,
	const struct MemInventory *inventory,
	const int *cpus,
	int num_cpus
)
{
	int before = selection->count;

	for (int i = 0; i < inventory->count; i++)
	{
		const struct MemEvent *event = &inventory->items[i];
		char base[256];
		char umask[128];

		event_base(event->name, base, sizeof(base));
		event_umask(event->name, umask, sizeof(umask));

		if (strncmp(base, "DRAM_CHANNEL_", 13) == 0)
		{
			if (umask[0] == '\0' || strcmp(umask, "ALL") == 0)
				selection_add_event(selection, event, 64.0, cpus, num_cpus);
		}
		else if (strcmp(base, "DRAM_ACCESSES") == 0)
		{
			if (umask[0] == '\0' || strcmp(umask, "ALL") == 0)
				selection_add_event(selection, event, 64.0, cpus, num_cpus);
		}
	}

	return selection->count - before;
}

static int select_power9(
	struct MemSelectionList *selection,
	const struct MemInventory *inventory,
	const int *cpus,
	int num_cpus
)
{
	int before = selection->count;

	for (int i = 0; i < inventory->count; i++)
	{
		const struct MemEvent *event = &inventory->items[i];
		char base[256];

		event_base(event->name, base, sizeof(base));

		if (contains(base, "PM_MBA") &&
		    (contains(base, "READ_BYTES") || contains(base, "WRITE_BYTES")))
			selection_add_event(selection, event, 64.0, cpus, num_cpus);
	}

	return selection->count - before;
}

static int select_power8(
	struct MemSelectionList *selection,
	const struct MemInventory *inventory,
	const int *cpus,
	int num_cpus
)
{
	static const char *const event_names[] = {
		"PM_DATA_ALL_FROM_MEMORY",
		"PM_L3_CO_MEPF",
		"PM_L3_PF_ON_CHIP_MEM",
	};
	int before = selection->count;

	for (int i = 0; i < inventory->count; i++)
	{
		const struct MemEvent *event = &inventory->items[i];
		char base[256];

		event_base(event->name, base, sizeof(base));

		for (size_t j = 0; j < sizeof(event_names) / sizeof(event_names[0]); j++)
		{
			if (strcmp(base, event_names[j]) == 0)
			{
				selection_add_event(selection, event, 128.0, cpus, num_cpus);
				break;
			}
		}
	}

	return selection->count - before;
}

static int select_arm_a64fx(
	struct MemSelectionList *selection,
	const struct MemInventory *inventory,
	const int *cpus,
	int num_cpus
)
{
	int before = selection->count;

	for (int i = 0; i < inventory->count; i++)
	{
		const struct MemEvent *event = &inventory->items[i];
		char pmu[128];
		char base[256];

		event_pmu(event->name, pmu, sizeof(pmu));
		event_base(event->name, base, sizeof(base));

		if (!contains(pmu, "a64fx"))
			continue;

		if (strcmp(base, "L2D_CACHE_REFILL") == 0 ||
		    strcmp(base, "L2D_CACHE_WB") == 0)
			selection_add_event(selection, event, 256.0, cpus, num_cpus);
		else if (strcmp(base, "L2D_SWAP_DM") == 0 ||
		         strcmp(base, "L2D_CACHE_MIBMCH_PRF") == 0)
			selection_add_event(selection, event, -256.0, cpus, num_cpus);
	}

	return selection->count - before;
}

static int select_arm_mem(
	struct MemSelectionList *selection,
	const struct MemInventory *inventory,
	const int *cpus,
	int num_cpus
)
{
	static const char *const event_names[] = {
		"MEM_ACCESS_RD",
		"MEM_ACCESS_WR",
		"MEMORY_READS",
		"MEMORY_WRITES",
	};
	int before = selection->count;

	for (int i = 0; i < inventory->count; i++)
	{
		const struct MemEvent *event = &inventory->items[i];
		char base[256];

		event_base(event->name, base, sizeof(base));

		for (size_t j = 0; j < sizeof(event_names) / sizeof(event_names[0]); j++)
		{
			if (strcmp(base, event_names[j]) == 0)
			{
				selection_add_event(selection, event, 64.0, cpus, num_cpus);
				break;
			}
		}
	}

	return selection->count - before;
}

static int select_arm_l2(
	struct MemSelectionList *selection,
	const struct MemInventory *inventory,
	const int *cpus,
	int num_cpus
)
{
	static const char *const event_names[] = {
		"L2D_CACHE_REFILL",
		"L2D_CACHE_WB",
	};
	int before = selection->count;

	for (int i = 0; i < inventory->count; i++)
	{
		const struct MemEvent *event = &inventory->items[i];
		char base[256];

		event_base(event->name, base, sizeof(base));

		for (size_t j = 0; j < sizeof(event_names) / sizeof(event_names[0]); j++)
		{
			if (strcmp(base, event_names[j]) == 0)
			{
				selection_add_event(selection, event, 64.0, cpus, num_cpus);
				break;
			}
		}
	}

	return selection->count - before;
}

typedef int (*select_function_t)(
	struct MemSelectionList *,
	const struct MemInventory *,
	const int *,
	int
);

struct Profile
{
	const char *name;
	select_function_t select;
};

static const struct Profile profiles[] = {
	{ "intel-imc", select_intel_imc },
	{ "intel-qmc", select_intel_qmc },
	{ "intel-dram", select_intel_dram },
	{ "intel-client", select_intel_client },
	{ "amd-umc", select_amd_umc },
	{ "amd-df", select_amd_df },
	{ "power9-mba", select_power9 },
	{ "power8", select_power8 },
	{ "arm-a64fx", select_arm_a64fx },
	{ "arm-mem", select_arm_mem },
	{ "arm-l2", select_arm_l2 },
};

static int env_int(const char *name, int fallback)
{
	const char *value = getenv(name);
	if (value == NULL || *value == '\0')
		return fallback;

	char *end = NULL;
	long parsed = strtol(value, &end, 10);
	if (end == value || *end != '\0' || parsed < 0 || parsed > 1000000)
		return fallback;

	return (int)parsed;
}

static double env_double(const char *name, double fallback)
{
	const char *value = getenv(name);
	if (value == NULL || *value == '\0')
		return fallback;

	char *end = NULL;
	double parsed = strtod(value, &end);
	if (end == value || *end != '\0')
		return fallback;

	return parsed;
}

static int read_socket_cpus(int *cpus, int max_cpus)
{
	int socket_ids[256];
	int socket_first_cpu[256];
	int num_sockets = 0;
	int num_cpus = 0;

	long online = sysconf(_SC_NPROCESSORS_ONLN);
	if (online <= 0)
		online = 1;
	if (online > 4096)
		online = 4096;

	for (long cpu = 0; cpu < online && num_sockets < (int)(sizeof(socket_ids) / sizeof(socket_ids[0])); cpu++)
	{
		char path[256];
		snprintf(path, sizeof(path),
			"/sys/devices/system/cpu/cpu%ld/topology/physical_package_id", cpu);

		FILE *f = fopen(path, "r");
		if (f == NULL)
			continue;

		int package = -1;
		int matched = fscanf(f, "%d", &package);
		fclose(f);
		if (matched != 1)
			continue;

		int found = -1;
		for (int i = 0; i < num_sockets; i++)
		{
			if (socket_ids[i] == package)
			{
				found = i;
				break;
			}
		}

		if (found < 0)
		{
			socket_ids[num_sockets] = package;
			socket_first_cpu[num_sockets] = (int)cpu;
			num_sockets++;
		}
	}

	for (int i = 0; i < num_sockets && num_cpus < max_cpus; i++)
		cpus[num_cpus++] = socket_first_cpu[i];

	if (num_cpus == 0 && max_cpus > 0)
		cpus[num_cpus++] = 0;

	return num_cpus;
}

static int detect_explicit(
	struct PerfMemBandwidthEvent *events,
	int max_events,
	int verbosity
)
{
	const char *list = getenv("PERF_REGIONS_BANDWIDTH_EVENTS");
	if (list == NULL || *list == '\0')
		return -1;

	char *copy = mem_strdup(list);
	double bytes_per_count = env_double("PERF_REGIONS_BANDWIDTH_BYTES_PER_COUNT", 64.0);
	int forced_cpu = env_int("PERF_REGIONS_BANDWIDTH_CPU", -1);
	int count = 0;

	char *saveptr = NULL;
	for (char *event = strtok_r(copy, ",", &saveptr);
	     event != NULL;
	     event = strtok_r(NULL, ",", &saveptr))
	{
		mem_trim(event);
		if (*event == '\0')
			continue;

		if (count >= max_events)
		{
			fprintf(stderr, PRINT_PREFIX"Too many explicit memory bandwidth events (max: %d)\n",
				max_events);
			for (int i = 0; i < count; i++)
				free(events[i].name);
			free(copy);
			return -1;
		}

		char name[1024];
		if (forced_cpu >= 0 && strstr(event, ":cpu=") == NULL)
			snprintf(name, sizeof(name), "%s:cpu=%d", event, forced_cpu);
		else
			snprintf(name, sizeof(name), "%s", event);

		events[count].name = mem_strdup(name);
		events[count].bytes_per_count = bytes_per_count;
		count++;

		if (verbosity > 0)
			printf(PRINT_PREFIX"Using explicit event '%s' (%.1f bytes/count)\n",
				name, bytes_per_count);
	}

	free(copy);
	return count;
}

int perf_mem_bandwidth_detect(
	struct PerfMemBandwidthEvent *events,
	int max_events,
	int verbosity
)
{
	if (events == NULL || max_events <= 0)
		return -1;

	int explicit_count = detect_explicit(events, max_events, verbosity);
	if (explicit_count >= 0)
		return explicit_count;

	struct MemInventory inventory = { 0 };
	inventory_build(&inventory);

	if (inventory.count == 0)
	{
		fprintf(stderr, PRINT_PREFIX"No PAPI native events were found.\n");
		inventory_free(&inventory);
		return 0;
	}

	int cpus[256];
	int num_cpus = read_socket_cpus(cpus, (int)(sizeof(cpus) / sizeof(cpus[0])));
	int forced_cpu = env_int("PERF_REGIONS_BANDWIDTH_CPU", -1);
	if (forced_cpu >= 0)
	{
		cpus[0] = forced_cpu;
		num_cpus = 1;
	}

	struct MemSelectionList selection = { 0 };
	const char *profile_name = getenv("PERF_REGIONS_BANDWIDTH_PROFILE");
	const struct Profile *selected_profile = NULL;

	if (profile_name != NULL && *profile_name != '\0')
	{
		for (size_t i = 0; i < sizeof(profiles) / sizeof(profiles[0]); i++)
		{
			if (strcmp(profile_name, profiles[i].name) == 0)
			{
				selected_profile = &profiles[i];
				break;
			}
		}

		if (selected_profile == NULL)
			fprintf(stderr, PRINT_PREFIX"Ignoring unknown PERF_REGIONS_BANDWIDTH_PROFILE '%s'\n",
				profile_name);
	}

	if (selected_profile != NULL)
	{
		selected_profile->select(&selection, &inventory, cpus, num_cpus);
	}
	else
	{
		for (size_t i = 0; i < sizeof(profiles) / sizeof(profiles[0]); i++)
		{
			int added = profiles[i].select(&selection, &inventory, cpus, num_cpus);
			if (added > 0)
			{
				selected_profile = &profiles[i];
				break;
			}
		}
	}

	if (selection.count == 0)
	{
		fprintf(stderr, PRINT_PREFIX"Could not detect memory bandwidth events for this system.\n");
		fprintf(stderr, PRINT_PREFIX"Use 'papi_native_avail' to inspect native events, or set\n");
		fprintf(stderr, PRINT_PREFIX"PERF_REGIONS_BANDWIDTH_EVENTS to an explicit event list.\n");
		selection_free_names(&selection);
		inventory_free(&inventory);
		return 0;
	}

	if (selection.count > max_events)
	{
		fprintf(stderr, PRINT_PREFIX"Detected %d memory events, but only %d fit (PERF_COUNTERS_MAX).\n",
			selection.count, max_events);
		selection_free_names(&selection);
		inventory_free(&inventory);
		return -1;
	}

	if (verbosity > 0)
	{
		const char *profile_label = selected_profile != NULL ? selected_profile->name : "explicit";
		printf(PRINT_PREFIX"Detected %d memory bandwidth events (profile: %s)\n",
			selection.count, profile_label);
		for (int i = 0; i < selection.count; i++)
			printf(PRINT_PREFIX"  %s (%.1f bytes/count)\n",
				selection.items[i].name, selection.items[i].bytes_per_count);
	}

	int result_count = selection.count;
	for (int i = 0; i < result_count; i++)
	{
		events[i].name = selection.items[i].name;
		selection.items[i].name = NULL;
		events[i].bytes_per_count = selection.items[i].bytes_per_count;
	}

	selection_free_names(&selection);
	inventory_free(&inventory);
	return result_count;
}

#endif
