#include "logging.h"
#include "options.h"
#include "parallel.h"

#include <time.h>

// program entry point
int main(int argc, char * argv[]) {
	struct timespec timer;
	struct options options_init;
	options_parse(argc, argv, &options_init);
	const struct options * const options = &options_init;

	// random seed
	srand((unsigned)time(NULL));

	// timer resolution
	clock_getres(CLOCK_MONOTONIC, &timer);
	verbose(options, "Timer resolution: %ld seconds, %lu nanoseconds\n",
			timer.tv_sec, timer.tv_nsec);

	spawnProcesses(options);

	return EXIT_SUCCESS;
}
