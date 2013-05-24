#include "csv.h"

#include <inttypes.h>
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
