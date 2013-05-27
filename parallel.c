#include "parallel.h"

#include "benchmarks.h"
#include "options.h"
#include "logging.h"

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
#ifndef NDEBUG
		fprintf(stderr, "process %lld forked\n", (long long)pid);
#endif // NDEBUG
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

	walk(options);

	// wait for children
	while (nchildren--)
		wait(NULL);
}

struct thread_context {
	const struct options * const options;
	pthread_barrier_t * ready;
	pthread_barrier_t * start;
};

void * runWalk(void * c) {
	const struct thread_context * const context = c;
	const struct options * const options = context->options;

	// TODO begin might 0
	//walking_t array_len = options->begin;
	walking_t array_len = options->step;
	struct walkArray * array;
	makeIncreasingWalkArray(array_len, &array);

	// setup done
	pthread_barrier_wait(context->ready);

	// timer started
	pthread_barrier_wait(context->start);

	walkArray(array, options->aaccesses, NULL);
	return NULL;
}

static inline nsec_t timespecToNsec(struct timespec * t) {
	return 1000 * 1000 * 1000 * t->tv_sec + t->tv_nsec;
}

// TODO: error correctness
void spawnThreads(const struct options * const options) {
	unsigned num = options->threads;
	pthread_t allthreads[num];
	pthread_barrier_t syncready;
	pthread_barrier_t syncstart;
	// let all threads wait until parent is ready to start timing
	pthread_barrier_init(&syncready, NULL, num + 1);
	pthread_barrier_init(&syncstart, NULL, num + 1);
	struct thread_context optarg = {
		options,
		&syncready,
		&syncstart
	};

	// start threads
	pthread_t * thread = allthreads;
	for (unsigned i = 0; i < num; ++i) {
		int rc = pthread_create(thread++, NULL, runWalk, &optarg);
		switch (rc) {
			case 0:
#ifndef NDEBUG
				printf("Thread %d created.\n", i + 1);
#endif
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

	struct timespec start, stop, elapsed;

	pthread_barrier_wait(&syncready);
	clock_gettime(CLOCK_MONOTONIC, &start);
	pthread_barrier_wait(&syncstart);

	// wait for threads to finish
	for (unsigned i = 0; i < num; ++i)
		pthread_join(allthreads[i], NULL);

	clock_gettime(CLOCK_MONOTONIC, &stop);
	elapsed.tv_sec = stop.tv_sec - start.tv_sec;
	elapsed.tv_nsec = stop.tv_nsec - start.tv_nsec;

	double totalbytes = num * (double)options->aaccesses * sizeof(walking_t);
	double tb_new = totalbytes / (double)timespecToNsec(&elapsed);

	printf("Total time: %"PRINSEC" nsec (%"PRINSEC" msec)\n",
			timespecToNsec(&elapsed),
			timespecToNsec(&elapsed) / (1000 * 1000));

	printf("Bandwidth: ~%.3lf GiB/s\n", tb_new);

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
