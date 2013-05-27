#pragma once

#include "arraywalk.h"
#include "options.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

typedef long long nsec_t;
#define PRINSEC "llu"
void CSV_LogTimings(
		FILE * log,
		long long id,
		struct walkArray * wa,
		nsec_t nsec,
		nsec_t stddev);

// user reporting
void verbose(const struct options * options, const char *format, ...);

// nanosecond conversion
static inline nsec_t timespecToNsec(struct timespec * t) {
	return 1000 * 1000 * 1000 * t->tv_sec + t->tv_nsec;
}
