#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "logging.hpp"
#include "options.hpp"
#include "parallel.hpp"

#include <type_traits>
// TODO is_trivially_copyable is not yet implemented in GCC/CLang/icpc
// --> enable static assertions for checks when support is available
// Meanwhile:
// A trivially copyable class is a class that (draft 3242, section [class]):
// - has no non-trivial copy constructors (12.8),
// - has no non-trivial move constructors (12.8),
// - has no non-trivial copy assignment operators (13.5.3, 12.8),
// - has no non-trivial move assignment operators (13.5.3, 12.8), and
// - has a trivial destructor (12.4).

#define REPORT_ERROR(ERNUM) fprintf(stderr, "%s: %d (%s) error: %s\n", __FILE__, __LINE__, __FUNCTION__, strerror(ERNUM))

#define CS_RETURN_ON_ERROR(fncall) do { \
		enum childSpawn_ret ret = fncall; \
		if (CS_SUCCESS != ret) \
			return ret; \
	} while(0)

/* Select read/write end from a pipe. */
#define PIPEREAD(P) P[0]
#define PIPEWRITE(P) P[1]

/* Process channels for processes belonging to a tree; see treeChildren(). */
struct channels {
	channels(
			const int rR = -1, const int rW = -1,
			const int fcR = -1, const int fcW = -1,
			const int tcR = -1, const int tcW = -1,
			const int tpR = -1, const int tpW = -1,
			const int fpR = -1, const int fpW = -1):
		root{rR, rW},
		fromChildren{fcR, fcW}, toChildren{tcR, tcW},
		toParent{tpR, tpW}, fromParent{fpR, fpW}
	{}

	channels(const int _root[2]) { channels(_root[0], root[1]); }

	int root[2];
	int fromChildren[2];
	int toChildren[2];
	int toParent[2];
	int fromParent[2];
};
#if 0 // see comment at '#include <type_traits>' at the top of this file
static_assert(std::is_trivially_copyable<channels>::value,
		"struct channels must be trivially copyable");
#else
static_assert(std::is_standard_layout<channels>::value,
		"struct channels must be trivially copyable");
#endif

/* Timing information structure to write to logger process. */
struct timings {
	pid_t pid    {-1};
	pid_t ppid   {-1};
	nsec_t init  {0};
	nsec_t ready {0};
	nsec_t start {0};
	nsec_t done  {0};
};
/* POSIX.1-2001: write(2)s <= PIPE_BUF bytes must be atomic. */
static_assert(sizeof(struct timings) <= PIPE_BUF,
              "PIPE_BUF too small: can not atomically send timing info.");
#if 0 // see comment at '#include <type_traits>' at the top of this file
static_assert(std::is_trivially_copyable<timings>::value,
		"struct timings must be trivially copyable");
#else
static_assert(std::is_standard_layout<timings>::value,
		"struct timings must be trivially copyable");
#endif

/* Processing raw timing information should happen outside of timed code. */
struct rawTimings {
	pid_t pid             {-1};
	pid_t ppid            {-1};
	struct timespec init  {0, 0};
	struct timespec ready {0, 0};
	struct timespec start {0, 0};
	struct timespec done  {0, 0};
};
#if 0 // see comment at '#include <type_traits>' at the top of this file
static_assert(std::is_trivially_copyable<rawTimings>::value,
		"struct rawTimings must be trivially copyable");
#else
static_assert(std::is_standard_layout<rawTimings>::value,
		"struct rawTimings must be trivially copyable");
#endif

static enum childSpawn_ret writeLoop(int, const void *, size_t);

static enum childSpawn_ret receiveMsg(const int[2], char);
static enum childSpawn_ret receiveReady(const int[2]);
static enum childSpawn_ret receiveStart(const int[2]);
static enum childSpawn_ret receiveDone(const int[2]);
static enum childSpawn_ret receiveRawTimings(const int[2],
                                             struct rawTimings * const);

static enum childSpawn_ret sendMsg(char, unsigned, const int[2]);
static enum childSpawn_ret sendReady(const int[2]);
static enum childSpawn_ret sendStart(unsigned, const int[2]);
static enum childSpawn_ret sendDone(const int[2]);
static enum childSpawn_ret sendRawTimings(const int[2],
                                          const struct rawTimings * const);

static enum childSpawn_ret treeChildren(unsigned, unsigned,
                                        const struct Options * const);
static enum childSpawn_ret treeChildren_loop(const unsigned, const unsigned,
                                             const pid_t, const int[2],
                                             const struct Options * const);
static inline void treeNode_ready(const bool,
                                  struct rawTimings * const,
                                  const struct channels * const);
static inline void treeNode_end(const bool, unsigned, unsigned, unsigned,
                                struct rawTimings * const,
                                const struct channels * const);

static void processRawTimings(const struct rawTimings * const,
                              struct timings * const);
static void printTimingsHeader(void);
static void printTimings(unsigned, unsigned, const struct timings * const);

typedef int threadSpawn(unsigned);
static threadSpawn linearThreads;
//static threadSpawn treeThreads;
nsec_t spawnThreads(const struct Options * const, thread_fn);

/*
 * As write(2), but only returns when the whole buffer has been written. Prints
 * a warning when more than one call to write(2) is needed, as communication
 * defined by this file should be performed atomically.
 * TODO: maybe designate a return value or a flag to control the output of the
 *       warning as needed
 */
static enum childSpawn_ret
writeLoop(const int fd, const void * buf, size_t count)
{
	do {
		ssize_t bytes = write(fd, buf, count);
		switch (bytes) {
		case (-1):
		case (0):
			return CSERR_WRITE;
		}
		count -= (size_t)bytes;
		if (count)
			fprintf(stderr, "WARN (%d): %s not performed atomically.\n",
			        getpid(), __FUNCTION__);
	} while (count > 0);
#ifndef NDEBUG
	/*
	 * Rather than risk an infinite loop and mess up completely, detect
	 * and report underflow.
	 */
	if (count) {
		fprintf(stderr, "BUG (%d): file %s, line %d\n",
		        getpid(), __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}
#endif /* !NDEBUG */
	return CS_SUCCESS;
}

/*
 * Pipe synchronization messages must be byte-sized for correctness, as there
 * will be multiple readers, and reading is not atomic.
 */
static const char msg_ready = 'r';
static const char msg_start = 's';
static const char msg_done = 'd';

/*
 * Reads a message from its first argument, and stores the result in its second
 * argument. Returns CS_SUCCESS on success. Otherwise returns CSERR_READ when
 * read produces an error, or CSERR_EOF when no data could be read.
 */
static enum childSpawn_ret
receiveMsg(const int ipc[2], const char expect)
{
#ifndef NDEBUG
	fprintf(stderr, "pid %d: %s (expect '%c')\n", getpid(), __FUNCTION__, expect);
#endif /* !NDEBUG */
	char result;
	// TODO: use select in debug mode
	ssize_t bytes = read(PIPEREAD(ipc), &result, sizeof(result));
	switch (bytes) {
	case (-1):
		return CSERR_READ;
	case (0):
		return CSERR_EOF;
	default:
		if (expect == result)
			return CS_SUCCESS;
		else
			return CSERR_EXPREADY;
	}
}

/*
 * Writes a message to its argument. Returns CS_SUCCESS on success. Otherwise
 * returns CSERR_WRITE when write produces an error, or no bytes could be read.
 */
static enum childSpawn_ret
sendMsg(const char msg, unsigned repeat, const int ipc[2])
{
#ifndef NDEBUG
	fprintf(stderr, "pid %d: %s '%c'\n", getpid(), __FUNCTION__, msg);
#endif /* !NDEBUG */
	switch (repeat) {
	case (0):
		return CS_SUCCESS;
	case (1):
		{
			ssize_t bytes = write(PIPEWRITE(ipc), &msg, sizeof(msg));
			switch (bytes) {
			case (-1):
			case (0):
				return CSERR_WRITE;
			default:
				return CS_SUCCESS;
			}
		}
	default:
		{
			char * msgbuf = static_cast<char *>(malloc(repeat));
			if (!msgbuf)
				return CSERR_MALLOC;

			(void)memset(msgbuf, msg, repeat);
			childSpawn_ret ret = writeLoop(PIPEWRITE(ipc), msgbuf, repeat);
			free(msgbuf);
			return ret;
		}
	}
}

static enum childSpawn_ret
receiveReady(const int ipc[2])
{
	return receiveMsg(ipc, msg_ready);
}

static enum childSpawn_ret
receiveStart(const int ipc[2])
{
	return receiveMsg(ipc, msg_start);
}

static enum childSpawn_ret
receiveDone(const int ipc[2])
{
	return receiveMsg(ipc, msg_done);
}

static enum childSpawn_ret
receiveRawTimings(const int ipc[2], struct rawTimings * rawTimings)
{
	size_t result_size = sizeof(*rawTimings);
	char * result = (char *)rawTimings;
	do {
		ssize_t bytes = read(PIPEREAD(ipc), result, result_size);
		switch (bytes) {
		case (-1):
			return CSERR_READ;
		case (0):
			return CSERR_EOF;
		}
		if (result_size == (size_t)bytes)
			return CS_SUCCESS;
		else {
			result_size -= (size_t)bytes;
			result += bytes;
		}
	} while (result_size > 0);
#ifndef NDEBUG
	/*
	 * Rather than risk an infinite loop and mess up completely, detect
	 * and report underflow.
	 */
	if (result_size) {
		fprintf(stderr, "BUG (%d): file %s, line %d\n",
		        getpid(), __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}
#endif /* !NDEBUG */
	return CS_SUCCESS;
}

static enum childSpawn_ret
sendReady(const int ipc[2])
{
	return sendMsg(msg_ready, 1, ipc);
}

static enum childSpawn_ret
sendStart(unsigned repeat, const int ipc[2])
{
	return sendMsg(msg_start, repeat, ipc);
}

static enum childSpawn_ret
sendDone(const int ipc[2])
{
	return sendMsg(msg_done, 1, ipc);
}

static enum childSpawn_ret
sendRawTimings(const int ipc[2], const struct rawTimings * rawTimings)
{
	return writeLoop(PIPEWRITE(ipc), rawTimings, sizeof(*rawTimings));
}

/* Create a process tree (or list when branch >= num). */
static enum childSpawn_ret
treeChildren(unsigned num, unsigned branch, const struct Options * const options)
{
	/* A branch of zero with non-zero children just doesn't make sense. */
	if (!branch) {
		if (num)
			return CSERR_NOBRANCH;
		else
			return CS_SUCCESS;
	}

	/* Buffered I/O could be duplicated by fork(): flush first. */
	fflush(stdout);

	pid_t root = getpid();
	int toRoot[2];
	if (pipe(toRoot)) {
		REPORT_ERROR(errno);
		return CSERR_PIPE;
	}
	return treeChildren_loop(num, branch, root, toRoot, options);
}

/* Use treeChildren; This helper should not be used directly. */
static enum childSpawn_ret
treeChildren_loop(const unsigned total, const unsigned branch,
                  const pid_t root, const int rootChannel[2],
                  const struct Options * const options)
{
	/*
	 * Pipes are used for simple ready/start IPC, as an alternative to using posix
	 * semaphores in shared memory. Pipes are shared between nodes at a given
	 * depth in the tree: one for 'ready' and one for 'start' messages. The
	 * channels are separate because otherwise two nodes at the same depth could
	 * read eachother's messages destined for their parent. Except for the root
	 * leaf nodes, two sets of these channels exist: one for the nodes directly
	 * above, and one for the nodes directly below. Channels are managed by
	 * parents: the 'above' channels are copies of the channels defined by a
	 * child's parent.
	 * Channel operation reminder: it is mandatory that no two kinds of messages
	 * can exist at the same time one a channel. If this is not the case, an error
	 * could be reported at runtime (when lucky). E.g. 'done' messages must not be
	 * sent by children before all children are ready. This is ensured because a
	 * 'start' message will only be sent after all ready messages (from all
	 * children in the tree) were received, and a 'done' message can not appear
	 * before a child has been instructed to start its job.
	 */
	channels channels(rootChannel);

	struct rawTimings rawTimings;
	pid_t pid = -1;
	unsigned remaining = total;

 treeChildren_while:
	clock_gettime(CLOCK_MONOTONIC, &rawTimings.init);
	while(remaining) {
		unsigned nchildren = remaining < branch ? remaining : branch;
		remaining -= nchildren;
		/* The first 'overflow' children will spawn an extra process. */
		unsigned overflow = remaining % branch;
		/* Aside from the overflow, spawn uniformly to keep the tree balanced. */
		remaining /= branch;

		/*
		 * TODO: when a child errors, the whole tree should fail
		 */
		if (pipe(channels.fromChildren) || pipe(channels.toChildren)) {
			REPORT_ERROR(errno);
			return CSERR_PIPE;
		}
		for (unsigned child = 0; child < nchildren; ++child) {
			pid = fork();
			switch (pid) {
			case (-1): /* Error: return immediately. */
				return CSERR_FORK;
			case (0):  /* Child: do some bookkeeping and continue main spawn loop. */
				if (child < overflow)
					++remaining;
				for (unsigned i = 0; i < 2; ++i) {
					/* Close pipe to grandparent and overwrite with parent pipe. */
					if (channels.toParent[i] > -1) {
						int err = close(channels.toParent[i]);
						err = close(channels.fromParent[i]) || err;
						if (err)
							REPORT_ERROR(err);
					}
					channels.toParent[i] = channels.fromChildren[i];
					channels.fromParent[i] = channels.toChildren[i];
				}
				goto treeChildren_while;
			}
		}
		/*
		 * Parent: wait for children and return pid of last child. Special cases for
		 * the root node: e.g.: it does not have to wait for a start message; when
		 * its children are ready, the benchmark can start.
		 */
		rawTimings.pid = getpid();
		rawTimings.ppid = getppid(); /* For logging purposes only. */
		const bool isRoot = root == rawTimings.pid;

		// TODO: setup threads
#ifndef NDEBUG
		printf("SETUP parent (pid %d)\n", rawTimings.pid);
#endif /* !NDEBUG */
		for (unsigned i = 0; i < nchildren; ++i)
			receiveReady(channels.fromChildren);
		treeNode_ready(isRoot, &rawTimings, &channels);

		sendStart(nchildren, channels.toChildren);
#ifndef NDEBUG
		printf("WORK parent (pid %d)\n", rawTimings.pid);
#endif /* NDEBUG */
		spawnThreads(options, runWalk); // TODO: extract spawn thr & treenode_end
		treeNode_end(isRoot, nchildren, branch, total, &rawTimings, &channels);
		return CS_SUCCESS;
	}
	/* Leaf node: no spawning, just work. */
	// TODO: extract rawtimings initialization code
	rawTimings.pid = getpid();
	rawTimings.ppid = getppid(); /* For logging purposes only. */
	const bool isRoot = root == rawTimings.pid;

	// TODO: setup threads
#ifndef NDEBUG
	printf("SETUP leaf (pid %d)\n", rawTimings.pid);
#endif /* !NDEBUG */
	treeNode_ready(isRoot, &rawTimings, &channels);

#ifndef NDEBUG
	printf("WORK leaf (pid %d)\n", rawTimings.pid);
#endif /* !NDEBUG */
	spawnThreads(options, runWalk);
	treeNode_end(isRoot, 0, branch, total, &rawTimings, &channels);
	return CS_SUCCESS;
}

/* Notify parent of ready state, and wait for start message. */
static inline void
treeNode_ready(bool isRoot, struct rawTimings * const rawTimings,
               const struct channels * const channels)
{
	if (!isRoot)
		sendReady(channels->toParent);
	clock_gettime(CLOCK_MONOTONIC, &rawTimings->ready);

	if (!isRoot)
		receiveStart(channels->fromParent);
	clock_gettime(CLOCK_MONOTONIC, &rawTimings->start);
}

/*
 * Actions to perform when a process tree node finishes work. Only the root node
 * returns, others call exit().
 */
static inline void
treeNode_end(bool isRoot,
             unsigned nchildren, unsigned branch, unsigned totalChildren,
             struct rawTimings * const rawTimings,
             const struct channels * const channels)
{
	struct timings timings;

	for (unsigned i = 0; i < nchildren; ++i)
		receiveDone(channels->fromChildren);

	if (!isRoot)
		sendDone(channels->toParent);

	clock_gettime(CLOCK_MONOTONIC, &rawTimings->done);

	/* Root must not write() its own timings; see receive loop below. */
	if (isRoot) {
		processRawTimings(rawTimings, &timings);
		printTimings(totalChildren, branch, &timings);
	} else {
		sendRawTimings(channels->root, rawTimings);
		exit(EXIT_SUCCESS);
	}

	/*
	 * Root must read() before any other potentially blocking calls like wait() or
	 * write(), otherwise the pipe may be full, creating a deadlock.
	 */
	for (unsigned child = 0; child < totalChildren; ++child) {
		if (receiveRawTimings(channels->root, rawTimings))
			REPORT_ERROR(errno);
		processRawTimings(rawTimings, &timings);
		printTimings(totalChildren, branch, &timings);
	}

	for (unsigned i = 0; i < nchildren; ++i) {
		int status;
		pid_t pid = wait(&status);
		if (-1 == pid)
			REPORT_ERROR(errno);
		else {
			if (!WIFEXITED(status))
				fprintf(stderr, "WARN: abnormal exit (%d)\n", pid);
			else
				if (WEXITSTATUS(status) != EXIT_SUCCESS)
					fprintf(stderr, "WARN: exit caused by failure (%d)\n", pid);
		}
	}

	/* Root doesn't exit: it must clean up its resources. */
	for (unsigned i = 0; i < 2; ++i) {
		int err = close(channels->root[i]);
		err = close(channels->fromChildren[i]) || err;
		err = close(channels->toChildren[i]) || err;
		if (err)
			REPORT_ERROR(errno);
	}
}

/* Convert raw timing information to our canonical representation. */
static void
processRawTimings(const struct rawTimings * const rawTimings,
                  struct timings * const timings)
{
	timings->pid = rawTimings->pid;
	timings->ppid = rawTimings->ppid;
	timings->init = timespecToNsec(&rawTimings->init);
	timings->ready = timespecToNsec(&rawTimings->ready);
	timings->start = timespecToNsec(&rawTimings->start);
	timings->done = timespecToNsec(&rawTimings->done);
}

/* Print timing header to standard output. */
static void
printTimingsHeader(void)
{
	printf("pid, parent, total, branch, init->done\n");
}

/* Print timing information to standard output. */
static void
printTimings(unsigned total, unsigned branch,
             const struct timings * const timings) {
#if 0
	printf("pid %d - init %" PRINSEC " - "
	       "ready %" PRINSEC " - start %" PRINSEC " - done %" PRINSEC "\n",
	       timings->pid, timings->init, timings->ready,
	       timings->start, timings->done);
	printf("\t=> init -> done in %" PRINSEC " msec\n",
	       (timings->done - timings->init) / (1000 * 1000));
#else
	printf("%d, %d, %u, %u, %" PRINSEC "\n", timings->pid, timings->ppid,
	       total, branch,
	       (timings->done - timings->init) / (1000 * 1000));
#endif
}

/* Spawn child processes according to configuration. */
static enum childSpawn_ret
spawnChildren(unsigned nchildren, unsigned nthreads, thread_fn benchmark __attribute__((unused)))
{
	int ipcpipe[2];
	if (0 != pipe(ipcpipe))
		return CSERR_PIPE;

	/* Spawn children; Children Wait until all processes have been spawned. */
	if (0 == treeChildren(nchildren, 2, NULL)) {
		/* Child: spawn the required amount of threads and synchronize. */
		pthread_barrier_t syncthr;
		pthread_barrier_init(&syncthr, NULL, nthreads + 1);
		int lt = linearThreads(nthreads);
		if (0 != lt)
			return CSERR_THREAD;
		sendReady(ipcpipe);
  } else {
		/* Parent: ensure all children are ready before running the benchmark. */
    for (unsigned i = 0; i < nchildren; ++i)
			CS_RETURN_ON_ERROR(receiveReady(ipcpipe));

		/* Global timers measure wall clock time for all processes to finish. */
    struct timespec start, finish;
		char * startbuf = static_cast<char *>(malloc(nchildren));
		if (!startbuf)
			return CSERR_MALLOC;
		memset(startbuf, msg_start, nchildren);
		clock_gettime(CLOCK_MONOTONIC, &start);
    CS_RETURN_ON_ERROR(writeLoop(PIPEWRITE(ipcpipe), startbuf, nchildren));
		for (unsigned i = 0; i < nchildren; ++i) {
			nsec_t thread_time = 0;
			read(PIPEREAD(ipcpipe), &thread_time, sizeof(thread_time));
			/*
			verbose(options,
			        "process finished; thread time: %" PRINSEC " msec\n",
			        thread_time / (1000 * 1000));
			*/
		}
		clock_gettime(CLOCK_MONOTONIC, &finish);
		finish.tv_sec -= start.tv_sec;
		finish.tv_nsec -= start.tv_nsec;
		/*
		verbose(options,
		        "%u processes; Total time: %" PRINSEC " msec\n",
		        num,
		        timespecToNsec(&finish) / (1000 * 1000));
		*/
	}
}

/*
 * Spawn num threads from the parent process. Return value is congruent with the
 * return value of pthread_create: 0 on success, error number otherwise. When
 * this function fails, the number of threads that have been spawned, and their
 * contents, are undefined.
 */
int
linearThreads(const unsigned num)
{
	thread_fn * foo = runWalk;
	pthread_t * threads = static_cast<pthread_t *>(malloc(sizeof(pthread_t) * num));
	int rc = 0;
	for (unsigned i = 0; i < num; ++i) {
		rc = pthread_create(threads + i, NULL, foo, NULL);
		switch (rc) {
		case 0:
			continue;
		default:
			goto linearThreads_end;
		}
	}
 linearThreads_end:
	free(threads);
	return rc;
}

nsec_t spawnThreads(const struct Options * const options, thread_fn benchmark) {
	const struct Options_generic * const gn_opt = &(options->generic);
	nsec_t retval = 0;

	for (unsigned num = gn_opt->threads_begin; num <= gn_opt->threads_end; ++num) {
		verbose(options, "\n>>> %u threads\n", num);

		pthread_t * allthreads = static_cast<pthread_t *>(malloc(sizeof(pthread_t) * num));
		pthread_barrier_t init, ready, set, go, finish;

		pthread_barrier_init(&init, NULL, num);
		pthread_barrier_init(&ready, NULL, num);
		pthread_barrier_init(&set, NULL, num);
		pthread_barrier_init(&go, NULL, num);
		pthread_barrier_init(&finish, NULL, num);

		struct Thread_context optargs = {
			options,
			num,
			NULL,
			&init, &ready, &set, &go, &finish
		};

		// start threads (must adhere to the 4-barrier protocol)
		pthread_t * thread = allthreads;
		for (unsigned i = 0; i < num; ++i) {
			int rc = pthread_create(thread++, NULL, benchmark, &optargs);
			switch (rc) {
				case 0:
					continue;
					break;
				case EAGAIN:
				case EINVAL:
				case EPERM:
					errno = rc;
					perror("pthread_create");
					free(allthreads);
					exit(EXIT_FAILURE);
					break;
				default:
					fprintf(stderr, "unknown error in pthread_create\n");
					free(allthreads);
					exit(EXIT_FAILURE);
					break;
			}
		}

		// wait for threads to finish and return
		nsec_t * thread_retval = NULL;
		for (unsigned i = 0; i < num; ++i) {
			pthread_join(allthreads[i], (void **)&thread_retval);
			if (thread_retval) {
				retval += *thread_retval;
				free(thread_retval);
			}
		}
		free(allthreads);
	}
	return retval;
}

// each process will create a number of threads that run synchronized
// processes can be instructed to start synchronosly (to the extent made
// possible using semaphores) but are otherwise unsynchronized until their
// job is completed
void spawnProcesses(const struct Options * const options) {
	//const struct Options_generic * const gn_opt = &(options->generic);

	const unsigned amount = 8;
	//const unsigned branch = 1000;
	const unsigned repeat = 1;

	printTimingsHeader();
	for (unsigned a = 1; a <= amount; ++a) {
		for (unsigned b = 1; b <= a; b *= 2) {
			for (unsigned rep = 1; rep <= repeat; ++rep) {
				enum childSpawn_ret ret = treeChildren(a, b, options);
				if (ret != CS_SUCCESS && ret != CSERR_NOBRANCH)
					REPORT_ERROR(errno);
				//else
				//printf("amount:branch = %u:%u PASS\n", a, b);
			}
		}
	}

	printf("pid %i says bye!\n", getpid());
	verbose(options, "Finished!\n");
	return;
}

/*******************************************************************************
 * threaded benchmarks below
 ******************************************************************************/

/**************
 * Walk Array *
 **************/
struct runWalkSharedData {
	struct WalkArray * array;
	nsec_t old_avg;
	nsec_t * timings;
};

static struct runWalkSharedData * makeRunWalkSharedData(
	const struct Options_walkarray * wa_opt)
{
	struct runWalkSharedData * init =
		static_cast<runWalkSharedData *>(malloc(sizeof(struct runWalkSharedData)));
	init->array = NULL;
	init->old_avg = 0;
	init->timings =
		static_cast<nsec_t *>(malloc(sizeof(*(init->timings)) * wa_opt->repetitions));
	return init;
}

static void freeRunWalkSharedData(struct runWalkSharedData * rwsd) {
	if (!rwsd)
		return;

	free(rwsd->timings);
	free(rwsd);
}

void * runWalk(void * c) {
	struct Thread_context * const context = static_cast<Thread_context *>(c);
	const struct Options * const options = context->options;
	const struct Options_walkarray * const wa_opt = &(options->walkArray);

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
			shared = static_cast<runWalkSharedData *>(context->shared);
			struct timespec t_wa = makeWalkArray(wa_opt->pattern, len, &(shared->array));
			logMakeWalkArray(options, shared->array, &t_wa);
		}
		pthread_barrier_wait(context->ready);
		/*- barrier --------------------------------------------------------------*/

		shared = static_cast<runWalkSharedData *>(context->shared);
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
			        "Total time: %" PRINSEC " nsec (%" PRINSEC " msec)\n",
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
	struct StreamArray * array;
	nsec_t old_avg;
};

static struct runStreamSharedData * makeRunStreamSharedData(
	const struct Options_streamarray * sa_opt __attribute__((unused)))
{
	struct runStreamSharedData * init =
		static_cast<runStreamSharedData *>(malloc(sizeof(struct runStreamSharedData)));
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
	struct Thread_context * const context = static_cast<Thread_context *>(c);
	const struct Options * const options = context->options;
	const struct Options_streamarray * const sa_opt = &(options->streamArray);

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
			shared = static_cast<runStreamSharedData *>(context->shared);
			makeStreamArray(I64, len, &(shared->array));
		}
		pthread_barrier_wait(context->ready);
		/*- barrier --------------------------------------------------------------*/

		shared = static_cast<runStreamSharedData *>(context->shared);

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
			        "Array size: %zd MiB | Total time: %" PRINSEC " nsec (%" PRINSEC " msec)\n",
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
	struct FlopsArray * array;
	nsec_t old_avg;
};

static struct runFlopsSharedData * makeRunFlopsSharedData(
	const struct Options_flopsarray * fa_opt __attribute__((unused)))
{
	struct runFlopsSharedData * init =
		static_cast<runFlopsSharedData *>(malloc(sizeof(struct runFlopsSharedData)));
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
	struct Thread_context * const context = static_cast<Thread_context *>(c);
	const struct Options * const options = context->options;
	const struct Options_flopsarray * const fa_opt = &(options->flopsArray);

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
	unsigned long long fraction = 0;
	unsigned len = 0 == fa_opt->begin ? fa_opt->step : fa_opt->begin;
	for ( ; len <= fa_opt->end ; len += fa_opt->step) {
		struct timespec t_go, t_finish, t_lap;
		if (PTHREAD_BARRIER_SERIAL_THREAD == init_serial_thread) {
			shared = static_cast<runFlopsSharedData *>(context->shared);
			makeFlopsArray(SINGLE, (int)len, &(shared->array));
			fraction = fa_opt->calculations / (len * context->nthreads);
		}
		pthread_barrier_wait(context->ready);
		/*- barrier --------------------------------------------------------------*/

		shared = static_cast<runFlopsSharedData *>(context->shared);
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
			        "Array size: %zd MiB | Total time: %" PRINSEC " nsec (%" PRINSEC " msec)\n",
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
		nsec_t * retval = static_cast<nsec_t *>(malloc(sizeof(nsec_t)));
		*retval = run_totalnsec;
		return retval;
	}

	return NULL;
}
