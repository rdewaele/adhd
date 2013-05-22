#include "util.h"
#include "arraywalk.h"
#include "csv.h"
#include "options.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// user reporting
static void verbose(const struct options * options, const char *format, ...) {
	va_list args;
	va_start(args, format);

	if(!options->Silent)
		vprintf(format, args);

	va_end(args);
}

static inline nsec_t timespecToNsec(struct timespec * t) {
	return 1000 * 1000 * 1000 * t->tv_sec + t->tv_nsec;
}

// test for increasing cache sizes
static void walk(struct options * options) {
	// pid used for logging
	const pid_t pid = getpid();
	// flag to close log file if we opened it ourselves
	bool openedFile = false;
	// TODO: remove csvlog from options, it doesn't make sense anymore for this
	// multithreaded program
	if (strlen(options->csvlogname) && !options->csvlog) {
		const char * name = options->csvlogname;
		char tmp[NAME_MAX];
		snprintf(tmp, NAME_MAX, "%s_%lld.csv", name, (long long)pid);
		options->csvlog = fopen(tmp, "w");
		openedFile = true;
	}

	// bookkeeping
	nsec_t timings[options->repetitions];
	struct timespec elapsed;
	nsec_t totalnsec;
	nsec_t new_avg = 0;
	nsec_t old_avg = 0;
	double stddev;
	walking_t array_len = 0;
	struct walkArray * array;

	// check for overflow before actually incrementing the array size
	while (WALKING_T_CAST(array_len + options->step) > array_len &&
			(array_len = WALKING_T_CAST(array_len + options->step)) <= options->end) {
		// array creation (timed)
		totalnsec = 0;
		elapsed = makeRandomWalkArray(array_len, &array);
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
		if (options->csvlog)
			CSV_LogTimings(options->csvlog, pid, array, new_avg, lround(stddev));

		verbose(options, ">>>\t%"PRINSEC" usec"
				" | delta %+2.2lf%%"
				" (%"PRINSEC" -> %"PRINSEC")"
				" | stddev %ld usec (%2.2lf%%)\n",
				new_avg / 1000,
				100 * (double)(new_avg - old_avg) / (double)old_avg,
				old_avg, new_avg,
				lround(stddev / 1000), 100 * stddev / (double)new_avg
				);
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
		verbose(options, "\n");

		// inform user in time about every iteration
		fflush(stdout);

		// prepare for next test instance
		old_avg = new_avg;
		freeWalkArray(array);
	}

	if (openedFile) {
		fclose(options->csvlog);
		options->csvlog = NULL;
	}
}

// create a new arraywalk child process
// XXX exit when child creation fails, as test will yield unexpected results
static pid_t spawnChildren(unsigned num) {
	pid_t pid = 0;
	while (num--) {
		switch ((pid = fork())) {
			case -1:
				perror("child process creation");
				exit(EXIT_FAILURE);
				break;
			case 0:
				// child: do not spawn
				goto stopSpawn;
				break;
			default:
				// parent: continue spawning
				break;
		}
	}
stopSpawn:
	return pid;
}

// create the desired amount of children all from the same parent
static void linearSpawn(struct options * options) {
	unsigned nchildren = options->processes;
	if(0 == spawnChildren(nchildren))
		walk(options);
	else
		while (nchildren--)
			wait(NULL);
}

// create children in a tree-like fashion; i.e. children creating children
// XXX assumes options.processes > 1
static void treeSpawn(struct options * options) {
	// TODO: maybe introduce support for configurable branching factors
	unsigned todo = options->processes - 1; // initial thread will also calculate
	unsigned nchildren = 0;
	do {
		switch ((nchildren = todo % 2)) {
			case 0:
				nchildren = 2;
			default:
				// nchildren unmodified
				break;
		}

		// if spawnChild returns, child creation was successful; i.e. pid >= 0
		// parent breaks out loop and starts calculating;
		// children continue to create their own children first
		todo = (todo - nchildren) / nchildren;
		if (spawnChildren(nchildren) > 0)
			break;

	} while (todo > 0);

	walk(options);

	// wait for children
	while (nchildren--)
		wait(NULL);
}

// program entry point
int main(int argc, char * argv[]) {
	struct timespec timer;
	struct options options;
	options_parse(argc, argv, &options);

	// print timer resolution
	clock_getres(CLOCK_MONOTONIC, &timer);
	verbose(&options, "Timer resolution: %ld seconds, %lu nanoseconds\n",
			timer.tv_sec, timer.tv_nsec);

	// nothing to do
	if (0 == options.processes)
		return EXIT_SUCCESS;

	// seed random at program startup
	srand((unsigned)time(NULL));

	if (1 == options.processes) {
		// single threaded run
		walk(&options);
	} else {
		// multithreaded run
		switch (options.create) {
			case TREE:
				treeSpawn(&options);
				break;
			case LINEAR:
				linearSpawn(&options);
				break;
		}
	}

	// valgrind-cleanliness
	options_free(&options);

	return EXIT_SUCCESS;
}
