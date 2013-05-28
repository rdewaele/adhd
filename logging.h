#pragma once

#include "benchmarks.h"
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

const char * bool2yesno(bool b);
const char * bool2onoff(bool b);

// walkArray creation report
void logMakeWalkArray(
		const struct options * const options,
		const struct walkArray * const array,
		struct timespec * elapsed);

// walkArray performance report
void logWalkArray(
		const struct options * const options,
		const nsec_t * const timings,
		nsec_t old_avg);
