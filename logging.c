#include "logging.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

void CSV_LogTimings(
		FILE * log,
		long long id,
		struct walkArray * wa,
		nsec_t nsec,
		nsec_t stddev
		)
{
	if (log)
		fprintf(log,
				"%lld,%zu,%llu,%llu\n",
				id, wa->size, nsec, stddev);
}

// log to stdout depending on verbosity
void verbose(const struct options * options, const char *format, ...) {
	va_list args;
	va_start(args, format);

	if(!options->Silent)
		vprintf(format, args);

	va_end(args);
}

// walkArray creation report
void logMakeWalkArray(
		const struct options * const options,
		const struct walkArray * const array,
		struct timespec * elapsed)
{
	const char * array_unit;
	walking_t array_size;
	const uint32_t kilo = 1 << 10;
	const uint32_t mega = 1 << 20;
	const uint32_t giga = 1 << 30;
	if (array->size < kilo) {
		array_unit = "B";
		array_size = array->size;
	}
	else if (array->size < mega) {
		array_unit = "KiB";
		array_size = array->size / kilo;
	}
	else if (array->size < giga) {
		array_unit = "MiB";
		array_size = array->size / mega;
	}
	else {
		array_unit = "GiB";
		array_size = array->size / giga;
	}
	nsec_t totalusec = timespecToNsec(elapsed) / 1000;
	verbose(options,
			"%.4lu %s (= %lu elements) randomized in %"PRINSEC" usec | %u reads:\n",
			array_size, array_unit, array->len, totalusec, options->aaccesses);
}
