#pragma once

#include "options.h"

#include <pthread.h>
#include <unistd.h>

// create new arraywalk child processes
// XXX exit when child creation fails, as test will yield unexpected results
pid_t spawnChildren_fork(unsigned num);

// add threads to the current process
bool spawnChildren_pthread(unsigned num, pthread_t * threads,
		void * (* start)(void *), void * arg);

// create the desired amount of children all from the same parent
void linearSpawn(const struct options * const options);

// create children in a tree-like fashion; i.e. children creating children
// XXX assumes options.processes > 1
void treeSpawn(const struct options * const options);

void spawnThreads(const struct options * const options);

void spawnProcesses(const struct options * const options);

// function type that can be used to start a pthread with
typedef void * thread_fn(void *);

// context for a pthread's main function
struct thread_context {
	// as defined in options.h
	const struct options * const options;
	// shared data
	void * shared;
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
thread_fn runStreaming;

// performs the 'flops' benchmark
thread_fn runFlops;

// benchmark launcher function type
typedef void launcher_fn(const struct options * const options);

launcher_fn launchWalk;
launcher_fn launchStream;
launcher_fn launchFlops;
