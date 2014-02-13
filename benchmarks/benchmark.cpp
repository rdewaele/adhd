#include "benchmark.hpp"

#include <cstring>
#include <stdexcept>
#include <system_error>
#include <mutex>

#include <pthread.h>
#include <unistd.h> // linux setaffinity: sysconf

using namespace std;

namespace adhd {
	/****************************************************************************/
	// SingleBenchmark

	SingleBenchmark::SingleBenchmark(): AffineStepper(0) {}

	/****************************************************************************/
	// ThreadedBenchmark

	// main thread function that will be passed to pthread_create, as method
	// pointers can (should/must) not be used; qualifying the function as
	// extern "C" maximises portability
	extern "C" {
		static void c_thread_main(ThreadedBenchmark::BenchmarkThread * bt) { bt->runThread(); }
	}
	static auto threadMain = reinterpret_cast<void * (*)(void *)>(c_thread_main);

	ThreadedBenchmark::ThreadedBenchmark(unsigned min, unsigned max):
		AffineStepper(min, max),
		callback_mutex(),
		tcb(),
		pthreadIDs(max),
		bmThreads(max),
		stopThreads(false),
		runningThreads(0),
		spin_go(0),
		spin_go_wait(0)
	{
		if (min < 1 || max < 1)
			throw invalid_argument("ThreadedBenchmark(): number of threads must be >= 1");
	}

	// clean up threads when class gets destructed (hide implementation detail)
	ThreadedBenchmark::~ThreadedBenchmark() { joinThreads(true); }

	// linux-specific way of setting thread affinity
	static inline void setaffinity_linux(const unsigned tnum, const pthread_t & thread) {
		// CPU_SET accepts int as its first argument, implying that the total
		// number of cores also must be <= INT_MAX
		const long sysret = sysconf(_SC_NPROCESSORS_ONLN);
		switch (sysret) {
			case -1:
				throw system_error(-1, generic_category(), strerror(errno));
				break;
			case 0:
				throw runtime_error("OS reports 0 processors online. What processor is this program even running on?");
				break;
			default:
				long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
#ifdef __ICC
				// XXX ICC spurious warning workaround - having a static_cast still
				// triggers an implicit conversion warning, a C-style cast doesn't
#define STATIC_CAST(t) (t)
#else
#define STATIC_CAST(t) static_cast<t>
#endif
				int core_id = STATIC_CAST(int)(tnum % num_cores);
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(core_id, &cpuset);
				pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
		}
	}

	void ThreadedBenchmark::init_barriers() {
		const unsigned nthr = numThreads();

		pthread_barrier_init(&runThreads_entry_b, NULL, nthr + 1);
		pthread_barrier_init(&runThreads_exit_b, NULL, nthr + 1);

		pthread_barrier_init(&init_b, NULL, nthr);
		pthread_barrier_init(&ready_b, NULL, nthr);
		pthread_barrier_init(&set_b, NULL, nthr);
		pthread_barrier_init(&go_b, NULL, nthr);
		pthread_barrier_init(&go_wait_b, NULL, nthr);
		pthread_barrier_init(&finish_b, NULL, nthr);
	}

	void ThreadedBenchmark::destroy_barriers() {
		pthread_barrier_destroy(&runThreads_entry_b);
		pthread_barrier_destroy(&runThreads_exit_b);

		pthread_barrier_destroy(&init_b);
		pthread_barrier_destroy(&ready_b);
		pthread_barrier_destroy(&set_b);
		pthread_barrier_destroy(&go_b);
		pthread_barrier_destroy(&go_wait_b);
		pthread_barrier_destroy(&finish_b);
	}

	inline void startWaitingThreads(pthread_barrier_t * b) {
		pthread_barrier_wait(b);
	}

	// threads are at a barrier when this method is called (or it's a bug)
	void ThreadedBenchmark::run(timing_cb newtcb) {
		// shared-memory assignment is OK because of this method's precondition
		tcb = newtcb;
		// join threads only if #threads changed (i.e. don't force)
		joinThreads(false);
		// if needed, spawn threads
		spawnThreads();
		// unblock all threads waiting to execute
		startWaitingThreads(&runThreads_entry_b);
		// block this method until all threads finished executing
		startWaitingThreads(&runThreads_exit_b);
	}

	void ThreadedBenchmark::spawnThreads() {
		const unsigned nthr = numThreads();

		if(!runningThreads) {
			init_barriers();
			for (unsigned t = 0; t < nthr; ++t) {
				bmThreads[t] = BenchmarkThread {t, this};
				const int rc = pthread_create(&pthreadIDs[t], NULL, threadMain, &bmThreads[t]);
				if (rc) { throw system_error(rc, generic_category(), strerror(rc)); }
				setaffinity_linux(t, pthreadIDs[t]);
			}
			runningThreads = nthr;
		}
	}

	void ThreadedBenchmark::joinThreads(bool force) {
		const unsigned nthr = numThreads();

		// only join threads when they are actually running
		if (runningThreads > 0)
			// only join running threads when forced (e.g. destructor) or the number
			// of active threads changed
			if (force || (runningThreads != nthr)) {
				stopThreads = true;
				startWaitingThreads(&runThreads_entry_b);

				for (unsigned t = 0; t < runningThreads; ++t)
					pthread_join(pthreadIDs[t], NULL);
				destroy_barriers();
				stopThreads = false;
				runningThreads = 0;
			}
	}

	ostream & operator<<(std::ostream & os, const ThreadedBenchmark & tb) {
		return os << "ThreadedBenchmark: " << static_cast<const AffineStepper<unsigned> &>(tb);
	}

	void ThreadedBenchmark::runThread(unsigned threadNum) {
		for (;;) {
			startWaitingThreads(&runThreads_entry_b);
			if (stopThreads) { return; }

			{ // init
				const int isSerial = pthread_barrier_wait(&init_b);
				if (PTHREAD_BARRIER_SERIAL_THREAD == isSerial) {
					spin_go = numThreads();
					spin_go_wait = numThreads();
					init(threadNum);
				} }
			{ // ready
				pthread_barrier_wait(&ready_b);
				ready(threadNum); }
			{ // set
				pthread_barrier_wait(&set_b);
				set(threadNum); }
			{ // go - always spin to keep calling go_wait_start() optional while
				// maintaining starting time spread as small as possible
				pthread_barrier_wait(&go_b);
				--spin_go;
				while(spin_go);
				go(threadNum); }
			{ // finish
				const int isSerial = pthread_barrier_wait(&finish_b);
				if (PTHREAD_BARRIER_SERIAL_THREAD == isSerial)
					finish(threadNum); }
			//break;
			startWaitingThreads(&runThreads_exit_b);
		}
	}

	// timing_callback serializes all calls to the callback supplied to run()
	void ThreadedBenchmark::timing_callback(const Timings & t) {
		auto && ul = unique_lock<mutex>(callback_mutex);
		tcb(t);
		ul.unlock();
	}

	// placeholders: no pure virtual methods to allow children to override no
	// more methods than they need, leaving only go() as abstract method
	void ThreadedBenchmark::init(unsigned) {}
	void ThreadedBenchmark::ready(unsigned) {}
	void ThreadedBenchmark::set(unsigned) {}
	void ThreadedBenchmark::finish(unsigned) {}
}
