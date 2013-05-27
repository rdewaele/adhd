#include "parallel.h"

#include "benchmarks.h"
#include "options.h"
#include "logging.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

// create new arraywalk child processes
// XXX exit when child creation fails, as test will yield unexpected results
pid_t spawnChildren_fork(unsigned num) {
	pid_t pid = 0;
	while (num--) {
		pid = fork();
		switch (pid) {
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
void linearSpawn(const struct options * const options) {
	unsigned nchildren = options->processes;
	if(0 == spawnChildren_fork(nchildren))
		// TODO: DEPRECATED
		walk(options);
	else
		while (nchildren--)
			wait(NULL);
}

// create children in a tree-like fashion; i.e. children creating children
// XXX assumes options.processes > 1
void treeSpawn(const struct options * const options) {
	// TODO: maybe introduce support for configurable branching factors
	unsigned todo = options->processes - 1; // initial process will also calculate
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
		if (spawnChildren_fork(nchildren) > 0)
			break;

	} while (todo > 0);

	// TODO: DEPRECATED
	walk(options);

	// wait for children
	while (nchildren--)
		wait(NULL);
}

void * runWalk(void * c) {
	const struct thread_context * const context = c;
	const struct options * const options = context->options;

	struct timespec elapsed;

	struct walkArray * array = NULL;
	walking_t len = 0 == options->begin ? options->step : options->begin;
	for ( ; len <= options->end ; len += options->step) {
		struct timespec t_wa = makeWalkArray(options->pattern, len, &array);
		logMakeWalkArray(options, array, &t_wa);

		// warmup run
		(void)walkArray(array, options->aaccesses, NULL);

		// indicate setup done
		if (context->ready) { pthread_barrier_wait(context->ready); }

		// wait for 'go' signal
		if (context->start) { pthread_barrier_wait(context->start); }

		// run timed code
		// TODO: option to enable or disable thread-local timing information
		if (/* options->thread_timing */ false) {
			//nsec_t timings[options->repetitions];
			for (size_t i = 0; i < options->repetitions; ++i) {
				walkArray(array, options->aaccesses, &elapsed);
				//timings[i] = timespecToNsec(&elapsed);
			}
		}
		else {
			for (size_t i = 0; i < options->repetitions; ++i) {
				walkArray(array, options->aaccesses, NULL);
			}
		}

		// indicate hot loop done
		if (context->stop) { pthread_barrier_wait(context->stop); }

		freeWalkArray(array);

		// overflow could cause infinite loop
		if (WALKING_MAX - options->step < len)
			break;
	}
	return NULL;
}

// TODO: error correctness
void spawnThreads(const struct options * const options) {
	unsigned num = options->threads;
	pthread_t allthreads[num];
	pthread_barrier_t syncready, syncstart, syncstop;

	pthread_barrier_init(&syncready, NULL, num + 1);
	pthread_barrier_init(&syncstart, NULL, num + 1);
	pthread_barrier_init(&syncstop, NULL, num + 1);

	struct thread_context optarg = {
		options,
		&syncready,
		&syncstart,
		&syncstop
	};

	// start threads
	pthread_t * thread = allthreads;
	for (unsigned i = 0; i < num; ++i) {
		int rc = pthread_create(thread++, NULL, runWalk, &optarg);
		switch (rc) {
			case 0:
				continue;
				break;
			case EAGAIN:
			case EINVAL:
			case EPERM:
			default:
				return;
				break;
		}
	}

	// time each of the runs of all threads
	walking_t iterations = (options->end - options->begin) / options->step;
	while (iterations--) {
		struct timespec start, stop, elapsed;

		pthread_barrier_wait(&syncready);
		pthread_barrier_destroy(&syncready);
		pthread_barrier_init(&syncready, NULL, num + 1);

		clock_gettime(CLOCK_MONOTONIC, &start);

		pthread_barrier_wait(&syncstart);
		pthread_barrier_destroy(&syncstart);
		pthread_barrier_init(&syncstart, NULL, num + 1);

		pthread_barrier_wait(&syncstop);
		clock_gettime(CLOCK_MONOTONIC, &stop);
		pthread_barrier_destroy(&syncstop);
		pthread_barrier_init(&syncstop, NULL, num + 1);

		elapsed.tv_sec = stop.tv_sec - start.tv_sec;
		elapsed.tv_nsec = stop.tv_nsec - start.tv_nsec;

		double totalbytes = num * options->repetitions * (double)options->aaccesses * sizeof(walking_t);
		double tb_new = totalbytes / (double)timespecToNsec(&elapsed);

		verbose(options,
				"Total time: %"PRINSEC" nsec (%"PRINSEC" msec)\n",
				timespecToNsec(&elapsed),
				timespecToNsec(&elapsed) / (1000 * 1000));

		verbose(options,
				"Bandwidth: ~%.3lf GiB/s\n", tb_new);
	}

	// wait for threads to finish
	for (unsigned i = 0; i < num; ++i)
		pthread_join(allthreads[i], NULL);
}

void spawnProcesses(const struct options * const options) {
	switch (options->processes) {
		case 0:
			return;
		case 1:
			spawnThreads(options);
			return;
		default:
			switch (options->create) {
				case TREE:
					treeSpawn(options);
					break;
				case LINEAR:
					linearSpawn(options);
					break;
			}
	}
}
