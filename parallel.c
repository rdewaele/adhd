#include "parallel.h"

#include "options.h"
#include "logging.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <sys/stat.h>
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
void linearSpawn(const struct options * const options, thread_fn benchmark, sem_t * syncstart) {
	const struct options_generic * const gn_opt = &(options->generic);
	int file[2];
	if (0 != pipe(file)) { perror("process pipe"); }

	for (unsigned nchildren = gn_opt->processes_begin; nchildren <= gn_opt->processes_end; ++nchildren) {
		verbose(options, "\n>>>>> %u processes\n", nchildren);
		// make sure output is written before duplicating the process
		fflush(stdout); fflush(stderr);
		if (0 == spawnChildren_fork(nchildren)) {
			sem_wait(syncstart);
			nsec_t thread_time = spawnThreads(options, benchmark);
			write(file[1], &thread_time, sizeof(thread_time));
			exit(EXIT_SUCCESS);
		}
		else {
			struct timespec start, finish;
			clock_gettime(CLOCK_MONOTONIC, &start);
			for (unsigned i = 0; i < nchildren; ++i) { sem_post(syncstart); }
			for (unsigned i = 0; i < nchildren; ++i) {
				wait(NULL);
				nsec_t thread_time = 0;
				read(file[0], &thread_time, sizeof(thread_time));
				verbose(options, "process finished; thread time: %"PRINSEC" msec\n", thread_time / (1000 * 1000));
			}
			clock_gettime(CLOCK_MONOTONIC, &finish);
			finish.tv_sec -= start.tv_sec;
			finish.tv_nsec -= start.tv_nsec;
			verbose(options,
					"%u processes; Total time: %"PRINSEC" msec\n",
					nchildren, timespecToNsec(&finish) / (1000 * 1000));
		}
	}
}

#if 0 // temporarily disabled until development time can be spent here
// create children in a tree-like fashion; i.e. children creating children
// XXX assumes options.processes > 1
void treeSpawn(const struct options * const options, thread_fn benchmark, sem_t * syncstart) {
	// TODO: maybe introduce support for configurable branching factors
	unsigned todo = options->generic.processes_begin - 1; // initial process will also calculate
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

	spawnThreads(options, benchmark);

	// wait for children
	while (nchildren--)
		wait(NULL);
}
#endif // temporarily disabled until development time can be spent here

// TODO: error correctness
nsec_t spawnThreads(const struct options * const options, thread_fn benchmark) {
	const struct options_generic * const gn_opt = &(options->generic);
	nsec_t retval = 0;

	for (unsigned num = gn_opt->threads_begin; num <= gn_opt->threads_end; ++num) {
		verbose(options, "\n>>> %u threads\n", num);

		pthread_t allthreads[num];
		pthread_barrier_t init, ready, set, go, finish;

		pthread_barrier_init(&init, NULL, num);
		pthread_barrier_init(&ready, NULL, num);
		pthread_barrier_init(&set, NULL, num);
		pthread_barrier_init(&go, NULL, num);
		pthread_barrier_init(&finish, NULL, num);

		struct thread_context optarg = {
			options,
			num,
			NULL,
			&init, &ready, &set, &go, &finish
		};

		// start threads (must adhere to the 4-barrier protocol)
		pthread_t * thread = allthreads;
		for (unsigned i = 0; i < num; ++i) {
			int rc = pthread_create(thread++, NULL, benchmark, &optarg);
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

		// wait for threads to finish and return
		nsec_t * thread_retval = NULL;
		for (unsigned i = 0; i < num; ++i) {
			pthread_join(allthreads[i], (void *)&thread_retval);
			if (thread_retval) {
				retval += *thread_retval;
				free(thread_retval);
			}
		}
	}
	return retval;
}

void spawnProcesses(const struct options * const options) {
	const struct options_generic * const gn_opt = &(options->generic);

	if (gn_opt->processes_begin <= 0)
		return;

	sem_t * syncstart = sem_open("/adhd_syncstart", O_CREAT | O_RDWR, S_IRWXU);
	sem_init(syncstart, !0, 0);
	switch (gn_opt->create) {
		case TREE:
			//treeSpawn(options, runFlops, syncstart);
			fprintf(stderr, "tree spawn currently unmaintaned, please check back later.\n");
			break;
		case LINEAR:
			linearSpawn(options, runFlops, syncstart);
			break;
	}
	if (0 != sem_destroy(syncstart))
		perror(NULL);
	if (0 != sem_close(syncstart))
		perror(NULL);
	verbose(options, "Finished!\n");
}

/*******************************************************************************
 * threaded benchmarks below
 ******************************************************************************/

/**************
 * Walk Array *
 **************/
struct runWalkSharedData {
	struct walkArray * array;
	nsec_t old_avg;
	nsec_t * timings;
};

static struct runWalkSharedData * makeRunWalkSharedData(
		const struct options_walkarray * wa_opt)
{
	struct runWalkSharedData * init = malloc(sizeof(struct runWalkSharedData));
	init->array = NULL;
	init->old_avg = 0;
	init->timings = malloc(sizeof(*(init->timings)) * wa_opt->repetitions);
	return init;
}

static void freeRunWalkSharedData(struct runWalkSharedData * rwsd) {
	if (!rwsd)
		return;

	free(rwsd->timings);
	free(rwsd);
}

void * runWalk(void * c) {
	struct thread_context * const context = c;
	const struct options * const options = context->options;
	const struct options_walkarray * const wa_opt = &(options->walkArray);

	// local alias for context->shared to prevent cast loitering
	// XXX note that this acts as a cached value!
	struct runWalkSharedData * shared = NULL;

	int init_serial_thread = pthread_barrier_wait(context->init);
	/*- barrier --------------------------------------------------------------*/
	if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread)
		context->shared = makeRunWalkSharedData(wa_opt);

	// time each of the runs of all threads
	walking_t len = 0 == wa_opt->begin ? wa_opt->step : wa_opt->begin;
	for ( ; len <= wa_opt->end ; len += wa_opt->step) {
		struct timespec t_go, t_finish, t_lap;
		if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread) {
			shared = context->shared;
			struct timespec t_wa = makeWalkArray(wa_opt->pattern, len, &(shared->array));
			logMakeWalkArray(options, shared->array, &t_wa);
		}
		pthread_barrier_wait(context->ready);
		/*- barrier --------------------------------------------------------------*/

		shared = context->shared;
		for (int i = 0; i < 3; ++i)
			(void)walkArray(shared->array, wa_opt->aaccesses, NULL);

		// run benchmark instance
		for (size_t i = 0; i < wa_opt->repetitions; ++i) {
			pthread_barrier_wait(context->set);
			/*- barrier ------------------------------------------------------------*/

			if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread)
				clock_gettime(CLOCK_MONOTONIC, &t_go); 

			pthread_barrier_wait(context->go);
			/*- barrier ------------------------------------------------------------*/

			walkArray(shared->array, wa_opt->aaccesses, NULL);

			pthread_barrier_wait(context->finish);
			/*- barrier ------------------------------------------------------------*/

			if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread) {
				// some bookkeeping of timing results
				clock_gettime(CLOCK_MONOTONIC, &t_finish);
				t_lap.tv_sec = t_finish.tv_sec - t_go.tv_sec;
				t_lap.tv_nsec = t_finish.tv_nsec - t_go.tv_nsec;
				shared->timings[i] = timespecToNsec(&t_lap);
			}
		}

		// benchmark instance cleanup and statistics
		if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread) {
			freeWalkArray(shared->array);

			shared->old_avg = logWalkArray(options, shared->timings, shared->old_avg);

			nsec_t totalnsec = 0;
			for (walking_t i = 0; i < wa_opt->repetitions; ++i)
				totalnsec += shared->timings[i];

			double totalbytes = (double)(sizeof(walking_t) * context->nthreads
					* wa_opt->repetitions * wa_opt->aaccesses);
			double tb_new = totalbytes / (double)totalnsec;

			verbose(options,
					"Total time: %"PRINSEC" nsec (%"PRINSEC" msec)\n",
					totalnsec, totalnsec / (1000 * 1000));

			verbose(options,
					"Bandwidth: ~%.3lf GiB/s\n", tb_new);
		}
	}
	if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread)
		freeRunWalkSharedData(shared);

	return NULL;
}

/*******************
 * Streaming Array *
 *******************/
struct runStreamSharedData {
	struct streamArray * array;
	nsec_t old_avg;
};

static struct runStreamSharedData * makeRunStreamSharedData(
		const struct options_streamarray * sa_opt __attribute__((unused)))
{
	struct runStreamSharedData * init = malloc(sizeof(struct runStreamSharedData));
	init->array = NULL;
	init->old_avg = 0;
	return init;
}

static void freeRunStreamSharedData(struct runStreamSharedData * rssd) {
	if (!rssd)
		return;

	free(rssd);
}

void * runStream(void * c) {
	struct thread_context * const context = c;
	const struct options * const options = context->options;
	const struct options_streamarray * const sa_opt = &(options->streamArray);

	// local alias for context->shared to prevent cast loitering
	// XXX note that this acts as a cached value!
	struct runStreamSharedData * shared = NULL;

	int init_serial_thread = pthread_barrier_wait(context->init);
	/*- barrier --------------------------------------------------------------*/
	if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread)
		context->shared = makeRunStreamSharedData(sa_opt);

	// time each of the runs of all threads
	unsigned len = 0 == sa_opt->begin ? sa_opt->step : sa_opt->begin;
	for ( ; len <= sa_opt->end ; len += sa_opt->step) {
		struct timespec t_go, t_finish, t_lap;
		if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread) {
			shared = context->shared;
			makeStreamArray(I64, len, &(shared->array));
		}
		pthread_barrier_wait(context->ready);
		/*- barrier --------------------------------------------------------------*/

		shared = context->shared;

		// run benchmark instance
		pthread_barrier_wait(context->set);
		/*- barrier ------------------------------------------------------------*/

		if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread)
			clock_gettime(CLOCK_MONOTONIC, &t_go); 

		pthread_barrier_wait(context->go);
		/*- barrier ------------------------------------------------------------*/

		// TODO
		//streamArray(shared->array);
		//memcpyArray(shared->array);
		fillArray(shared->array);
		//memsetArray(shared->array);

		pthread_barrier_wait(context->finish);
		/*- barrier ------------------------------------------------------------*/

		if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread) {
			// some bookkeeping of timing results
			clock_gettime(CLOCK_MONOTONIC, &t_finish);

			freeStreamArray(shared->array);

			t_lap.tv_sec = t_finish.tv_sec - t_go.tv_sec;
			t_lap.tv_nsec = t_finish.tv_nsec - t_go.tv_nsec;
			nsec_t totalnsec = timespecToNsec(&t_lap);

			// TODO: scale the amount of bytes fetched per thread

			verbose(options,
					"Array size: %zd MiB | Total time: %"PRINSEC" nsec (%"PRINSEC" msec)\n",
					shared->array->size / (1024 * 1024), totalnsec, totalnsec / (1000 * 1000));

			verbose(options,
					"Bandwidth: ~%.3lf GiB/s\n",
					(double)shared->array->size / (double)totalnsec);
		}
	}
	if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread)
		freeRunStreamSharedData(shared);

	return NULL;
}

/***************
 * Flops Array *
 ***************/
struct runFlopsSharedData {
	struct flopsArray * array;
	nsec_t old_avg;
};

static struct runFlopsSharedData * makeRunFlopsSharedData(
		const struct options_flopsarray * fa_opt __attribute__((unused)))
{
	struct runFlopsSharedData * init = malloc(sizeof(struct runFlopsSharedData));
	init->array = NULL;
	init->old_avg = 0;
	return init;
}

static void freeRunFlopsSharedData(struct runFlopsSharedData * rssd) {
	if (!rssd)
		return;

	free(rssd);
}
void * runFlops(void * c) {
	struct thread_context * const context = c;
	const struct options * const options = context->options;
	const struct options_flopsarray * const fa_opt = &(options->flopsArray);

	// contains timing information of the last run
	nsec_t run_totalnsec = 0;

	// local alias for context->shared to prevent cast loitering
	// XXX note that this acts as a cached value!
	struct runFlopsSharedData * shared = NULL;

	int init_serial_thread = pthread_barrier_wait(context->init);
	/*- barrier --------------------------------------------------------------*/
	if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread)
		context->shared = makeRunFlopsSharedData(fa_opt);

	// time each of the runs of all threads
	unsigned len = 0 == fa_opt->begin ? fa_opt->step : fa_opt->begin;
	for ( ; len <= fa_opt->end ; len += fa_opt->step) {
		struct timespec t_go, t_finish, t_lap;
		if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread) {
			shared = context->shared;
			makeFlopsArray(SINGLE, (int)len, &(shared->array));
		}
		pthread_barrier_wait(context->ready);
	/*- barrier --------------------------------------------------------------*/

		shared = context->shared;
		// warmup
		flopsArray(MADD, shared->array, len);

		// run benchmark instance
		pthread_barrier_wait(context->set);
		/*- barrier ------------------------------------------------------------*/

		if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread)
			clock_gettime(CLOCK_MONOTONIC, &t_go);

		pthread_barrier_wait(context->go);
		/*- barrier ------------------------------------------------------------*/

		// TODO
		unsigned long long fraction = fa_opt->calculations / (len * context->nthreads);
		flopsArray(MADD, shared->array, fraction);
		//flops_madd16(42, fa_opt->calculations);

		pthread_barrier_wait(context->finish);
		/*- barrier ------------------------------------------------------------*/

		if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread) {
			// some bookkeeping of timing results
			clock_gettime(CLOCK_MONOTONIC, &t_finish);

			t_lap.tv_sec = t_finish.tv_sec - t_go.tv_sec;
			t_lap.tv_nsec = t_finish.tv_nsec - t_go.tv_nsec;
			nsec_t totalnsec = timespecToNsec(&t_lap);
			run_totalnsec += totalnsec;

			// TODO: scale the amount of bytes fetched per thread

			verbose(options,
					"Iterations: %u\n", fraction);

			verbose(options,
					"Array size: %zd MiB | Total time: %"PRINSEC" nsec (%"PRINSEC" msec)\n",
					shared->array->size / (1024 * 1024), totalnsec, totalnsec / (1000 * 1000));

			double totalsec = (double)totalnsec / (1000. * 1000. * 1000.);
			verbose(options,
					"Flops: %u elements * %llu array iterations per thread * %u threads / %lf sec"
					" = %lf gigaflops\n",
					len, fraction, context->nthreads, totalsec,
					(double)(len * fraction * context->nthreads) / (double)totalnsec);
			verbose(options,
					"Bandwidth: ~%.3lf GiB/s\n",
					(double)fraction *
					(double)shared->array->size / (double)totalnsec);

			freeFlopsArray(shared->array);
		}
	}
	if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread) {
		freeRunFlopsSharedData(shared);
		nsec_t * retval = malloc(sizeof(nsec_t));
		*retval = run_totalnsec;
		return retval;
	}

	return NULL;
}
