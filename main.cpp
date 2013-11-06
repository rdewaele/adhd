#include "logging.hpp"
#include "options.hpp"
#include "parallel.hpp"

#include <time.h>

// program entry point
int main(int argc, char * argv[]) {
	struct timespec timer;
	struct Options options_init;
	options_parse(argc, argv, &options_init);
	const struct Options * const options = &options_init;

	// random seed
	srand((unsigned)time(NULL));

	// timer resolution
	clock_getres(CLOCK_MONOTONIC, &timer);
	verbose(options, "Timer resolution: %ld seconds, %lu nanoseconds\n",
			timer.tv_sec, timer.tv_nsec);

	spawnProcesses(options);

	return EXIT_SUCCESS;
}
