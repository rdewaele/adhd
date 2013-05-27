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

// performs the 'walk' benchmark
thread_fn runWalk;
