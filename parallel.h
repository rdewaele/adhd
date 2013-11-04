#pragma once

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include "options.h"
#include "logging.h"

/* Return codes for linearChildren and treeChildren. */
enum childSpawn_ret {
	CS_SUCCESS,
	CSERR_BOUNDS,
	CSERR_CLOSE,
	CSERR_EOF,
	CSERR_EXPREADY,
	CSERR_EXPSTART,
	CSERR_FORK,
	CSERR_MALLOC,
	CSERR_NOBRANCH,
	CSERR_PIPE,
	CSERR_READ,
	CSERR_THREAD,
	CSERR_WRITE
};

/* Create process(es) and launch benchmarks as defined by the options. */
void spawnProcesses(const struct options * const options);

// function type that can be used to start a pthread with
typedef void * thread_fn(void *);

// context for a pthread's main function
struct thread_context {
	// as defined in options.h
	const struct options * const options;
	// number of threads using this context
	unsigned nthreads;
	// shared data
	void * shared;
	// all threads initialized
	pthread_barrier_t * init;
	// shared benchmark data setup ready
	pthread_barrier_t * ready;
	// hot threads setup ready (may depend on data)
	pthread_barrier_t * set;
	// run hot loop, which should be timed
	pthread_barrier_t * go;
	// thread finished hot loop (stop timer when all threads finished)
	pthread_barrier_t * finish;
};

// performs the 'walk' benchmark
thread_fn runWalk;

// performs the 'streaming' benchmark
thread_fn runStream;

// performs the 'flops' benchmark
thread_fn runFlops;

// benchmark launcher function type
typedef void launcher_fn(const struct options * const options);

launcher_fn launchWalk;
launcher_fn launchStream;
launcher_fn launchFlops;
