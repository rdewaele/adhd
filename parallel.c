#include "parallel.h"

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
	unsigned nchildren = options->generic.processes;
	if(0 == spawnChildren_fork(nchildren))
		spawnThreads(options);
	else
		while (nchildren--)
			wait(NULL);
}

// create children in a tree-like fashion; i.e. children creating children
// XXX assumes options.processes > 1
void treeSpawn(const struct options * const options) {
	// TODO: maybe introduce support for configurable branching factors
	unsigned todo = options->generic.processes - 1; // initial process will also calculate
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

	spawnThreads(options);

	// wait for children
	while (nchildren--)
		wait(NULL);
}

void spawnThreads(const struct options * const options) {
	launchWalk(options);
}

// TODO: error correctness
void launchWalk(const struct options * const options) {
	const struct options_walkarray * const wa_opt = &(options->walkArray);
	const struct options_generic * const gn_opt = &(options->generic);

	unsigned num = gn_opt->threads;
	pthread_t allthreads[num];
	pthread_barrier_t ready, set, go, finish;

	const unsigned contenders = num + 1;
	pthread_barrier_init(&ready, NULL, contenders);
	pthread_barrier_init(&set, NULL, contenders);
	pthread_barrier_init(&go, NULL, contenders);
	pthread_barrier_init(&finish, NULL, contenders);

	struct thread_context optarg = {
		options,
		NULL,
		&ready, &set, &go, &finish
	};

	// start threads (must adhere to the 4-barrier protocol)
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
				errno = rc;
				perror("pthread_create");
				exit(EXIT_FAILURE);
				break;
			default:
				fprintf(stderr, "unknown error in pthread_create\n");
				exit(EXIT_FAILURE);
				break;
		}
	}

	// time each of the runs of all threads
#define WALKARRAY_FOREACH_LENGTH(OPT_WA,LENGTH) \
	LENGTH = 0 == OPT_WA->begin ? OPT_WA->step : OPT_WA->begin; \
	for ( ; LENGTH <= OPT_WA->end ; LENGTH += OPT_WA->step)

	struct walkArray ** array = (void *)&(optarg.shared);

	walking_t len;
	WALKARRAY_FOREACH_LENGTH(wa_opt, len) {
		struct timespec t_wa = makeWalkArray(wa_opt->pattern, len, array);
		logMakeWalkArray(options, *array, &t_wa);

		struct timespec start, stop, elapsed;
		nsec_t old_avg = 0;

	 	// (send) shared data ready
		pthread_barrier_wait(&ready);
		fprintf(stderr, "master passed ready\n");
		nsec_t timings[wa_opt->repetitions];
		for (unsigned i = 0; i < wa_opt->repetitions; ++i) {
			struct timespec t_go, t_finish, t_lap;

			// (receive) threads primed
			pthread_barrier_wait(&set);
			fprintf(stderr, "master passed set\n");
			clock_gettime(CLOCK_MONOTONIC, &t_go); 

			// (send) timer started
			pthread_barrier_wait(&go);
			fprintf(stderr, "master passed go\n");

			// (receive) threads finished
			pthread_barrier_wait(&finish);
			fprintf(stderr, "master passed finished\n");
			clock_gettime(CLOCK_MONOTONIC, &t_finish);

			// some bookkeeping of timing results
			t_lap.tv_sec = t_finish.tv_sec - t_go.tv_sec;
			t_lap.tv_nsec = t_finish.tv_nsec - t_go.tv_nsec;
			timings[i] = timespecToNsec(&t_lap);
		}

		freeWalkArray(*array);

		old_avg = logWalkArray(options, timings, old_avg);

		// benchmark instance statistics
		elapsed.tv_sec = stop.tv_sec - start.tv_sec;
		elapsed.tv_nsec = stop.tv_nsec - start.tv_nsec;

		double totalbytes = num * wa_opt->repetitions * (double)wa_opt->aaccesses * sizeof(walking_t);
		double tb_new = totalbytes / (double)timespecToNsec(&elapsed);

		verbose(options,
				"Total time: %"PRINSEC" nsec (%"PRINSEC" msec)\n",
				timespecToNsec(&elapsed),
				timespecToNsec(&elapsed) / (1000 * 1000));

		verbose(options,
				"Bandwidth: ~%.3lf GiB/s\n", tb_new);
	}

	// wait for threads to finish and return
	for (unsigned i = 0; i < num; ++i)
		pthread_join(allthreads[i], NULL);
}

void spawnProcesses(const struct options * const options) {
	const struct options_generic * const gn_opt = &(options->generic);

	switch (gn_opt->processes) {
		case 0:
			return;
		case 1:
			spawnThreads(options);
			return;
		default:
			switch (gn_opt->create) {
				case TREE:
					treeSpawn(options);
					break;
				case LINEAR:
					linearSpawn(options);
					break;
			}
	}
}

/*******************************************************************************
 * threaded benchmarks below
 */

void * runWalk(void * c) {
	const struct thread_context * const context = c;
	const struct options * const options = context->options;
	const struct options_walkarray * const wa_opt = &(options->walkArray);

	struct walkArray * const array = context->shared;

	// (receive) data ready
	pthread_barrier_wait(context->ready);
		fprintf(stderr, "passed ready\n");

	// warmup run
	fprintf(stderr, "array address: %p\n", array);
	(void)walkArray(array, wa_opt->aaccesses, NULL);

	for (size_t i = 0; i < wa_opt->repetitions; ++i) {
		// (send) primed
		pthread_barrier_wait(context->set);
		fprintf(stderr, "passed set\n");

		// (receive) timer started
		pthread_barrier_wait(context->go);
		fprintf(stderr, "passed go\n");

		walkArray(array, wa_opt->aaccesses, NULL);

		// (send) run finished
		pthread_barrier_wait(context->finish);
		fprintf(stderr, "passed finish\n");
	}

	return NULL;
}

void * runStreaming(void * c) {
	const struct thread_context * const context = c;
	const struct options * const options = context->options;
	const struct options_streamarray * const sa_opt = &(options->streamArray);

	return NULL;
}

void * runFlops(void * c) {
	const struct thread_context * const context = c;
	const struct options * const options = context->options;
	const struct options_flopsarray * const fa_opt = &(options->flopsArray);

	return NULL;
}
