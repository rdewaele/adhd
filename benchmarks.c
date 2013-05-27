#include "benchmarks.h"

#include "logging.h"
#include "parallel.h"
#include "options.h"

#include <assert.h>
#include <math.h>
#include <time.h>

// test for increasing cache sizes
void walk(const struct options * const options) {
	// logging setup
	const pid_t pid = getpid();
	FILE * csvlog = NULL;

	if (options->logging) {
		char tmp[NAME_MAX];
		snprintf(tmp, NAME_MAX, "%s_%lld.csv", options->csvlogname, (long long)pid);
		csvlog = fopen(tmp, "w");
	}

	// bookkeeping
	nsec_t timings[options->repetitions];
	struct timespec elapsed;
	nsec_t totalnsec;
	nsec_t new_avg = 0;
	nsec_t old_avg = 0;
	double stddev;
	walking_t array_len = options->begin;
	struct walkArray * array;

	// check for overflow before actually incrementing the array size
	while (WALKING_T_CAST(array_len + options->step) > array_len &&
			(array_len = WALKING_T_CAST(array_len + options->step)) <= options->end) {
		// array creation (timed)
		totalnsec = 0;
		switch (options->pattern) {
			case INCREASING:
				elapsed = makeIncreasingWalkArray(array_len, &array);
				break;
			case DECREASING:
				elapsed = makeDecreasingWalkArray(array_len, &array);
				break;
			case RANDOM:
				elapsed = makeRandomWalkArray(array_len, &array);
				break;
		}
		assert(isFullCycle(array->array, array_len));
		totalnsec += timespecToNsec(&elapsed);
		if (array->size < 1024)
			verbose(options, "%.6lu B", array->size);
		else
			verbose(options, "%.6lu KiB", array->size / 1024);

		verbose(options, " (= %lu elements) randomized in %"PRINSEC" usec | %lu reads:\n",
				array->len,
				totalnsec / 1000,
				options->aaccesses);

		// test case warmup (helps reducing variance)
		walkArray(array, options->aaccesses, &elapsed);
		// test each case 'repetions' times (timed)
		walking_t fidx = 0;
		for (size_t i = 0; i < options->repetitions; ++i) {
			fidx = walkArray(array, options->aaccesses, &elapsed);
			timings[i] = timespecToNsec(&elapsed);
		}

		// average
		totalnsec = 0;
		for (size_t i = 0; i < options->repetitions; ++i)
			totalnsec += timings[i];
		// XXX whole division should be OK: timings are in the millions of nsec
		new_avg = totalnsec / options->repetitions;
		if (0 == old_avg)
			old_avg = new_avg;

		// standard deviation
		totalnsec = 0;
		for (size_t i = 0; i < options->repetitions; ++i) {
			nsec_t current = timings[i];
			current = current > new_avg ? current - new_avg : new_avg - current;
			totalnsec += current * current;
		}
		stddev = sqrt((double)(totalnsec / options->repetitions));

		// report results
		CSV_LogTimings(csvlog, pid, array, new_avg, lround(stddev));

		// timing for a single run
		verbose(options, ">>>\t%"PRINSEC" usec"
				" | delta %+2.2lf%%"
				" (%"PRINSEC" -> %"PRINSEC")"
				" | stddev %ld usec (%2.2lf%%)\n",
				new_avg / 1000,
				100 * (double)(new_avg - old_avg) / (double)old_avg,
				old_avg, new_avg,
				lround(stddev / 1000), 100 * stddev / (double)new_avg
				);

		// timing for a single read
		nsec_t nsread_old = old_avg / options->aaccesses;
		nsec_t nsread_new = new_avg / options->aaccesses;
		verbose(options, ">>>\t~%"PRINSEC" nsec/read"
				" | delta %+2.2lf%%"
				" (%"PRINSEC" -> %"PRINSEC")"
				" | ~%.2lf cycles/read"
				" @ %.3f GHz"
				" | final index: %"PRIWALKING"\n",
				nsread_new,
				100 * (double)(nsread_new - nsread_old) / (double)nsread_old,
				nsread_old, nsread_new,
				((double)options->frequency * (double)new_avg) / (double)options->aaccesses,
				options->frequency,
				fidx);

		// bandwidth estimation for a single run
		double totalbytes = (double)options->aaccesses * sizeof(walking_t);
		double tb_new = totalbytes / (double)new_avg;
		double tb_old = totalbytes / (double)old_avg;
		verbose(options, ">>>\t~%.3lf GiB/s"
				" | delta %+2.2lf%%"
				" (%.3lf -> %.3lf)\n",
				tb_new,
				100 * (tb_new - tb_old) / tb_old,
				tb_old, tb_new);

		verbose(options, "\n");

		// inform user in time about every iteration
		fflush(stdout);

		// prepare for next test instance
		old_avg = new_avg;
		freeWalkArray(array);
	}

	if (csvlog) {
		fclose(csvlog);
		csvlog = NULL;
	}
}


