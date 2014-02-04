#include "benchmark.hpp"

#include <cstring>
#include <stdexcept>
#include <system_error>

#include <pthread.h>
#include <unistd.h> // linux setaffinity: sysconf

using namespace std;

namespace adhd {
	/*
	 * SingleBenchmark
	 */
	SingleBenchmark::SingleBenchmark(): reset(false) {}

	void SingleBenchmark::setUp() { runSingle(); }
	void SingleBenchmark::tearDown() {}

	void SingleBenchmark::next() { reset = !reset; }
	bool SingleBenchmark::atMin() const { return true; }
	bool SingleBenchmark::atMax() const { return true; }
	void SingleBenchmark::gotoBegin() { reset = false; }
	void SingleBenchmark::gotoEnd() { reset = true; }
	bool SingleBenchmark::equals(const RangeInterface & ri) const {
		const SingleBenchmark & sb = dynamic_cast<const SingleBenchmark &>(ri);
		return reset == sb.reset;
	}

	ostream & SingleBenchmark::toOStream(ostream & os) const {
		return os << "SingleBenchmark instance (" << reset << ")" << endl;
	}

	/*
	 * ThreadedBenchmark
	 */

	// main thread function that will be passed to pthread_create, as method
	// pointers can (should/must) not be used; qualifying the function as
	// extern "C" maximises portability
	extern "C" {
		static void c_thread_main(ThreadedBenchmark::BenchmarkThread * bt) { bt->runThread(); }
	}
	static auto threadMain = reinterpret_cast<void * (*)(void *)>(c_thread_main);

	ThreadedBenchmark::ThreadedBenchmark(unsigned min, unsigned max):
		pthreadIDs(max),
		bmThreads(max),
		threadRange(min, max),
		//stopThreads(false),
		spin_go(0),
		spin_go_wait(0)
	{}

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

		pthread_barrier_init(&runThreads_b, NULL, nthr + 1);

		pthread_barrier_init(&init_b, NULL, nthr);
		pthread_barrier_init(&ready_b, NULL, nthr);
		pthread_barrier_init(&set_b, NULL, nthr);
		pthread_barrier_init(&go_b, NULL, nthr);
		pthread_barrier_init(&go_wait_b, NULL, nthr);
		pthread_barrier_init(&finish_b, NULL, nthr);
	}

	void ThreadedBenchmark::destroy_barriers() {
		pthread_barrier_destroy(&runThreads_b);

		pthread_barrier_destroy(&init_b);
		pthread_barrier_destroy(&ready_b);
		pthread_barrier_destroy(&set_b);
		pthread_barrier_destroy(&go_b);
		pthread_barrier_destroy(&go_wait_b);
		pthread_barrier_destroy(&finish_b);
	}

	void ThreadedBenchmark::setUp() {
		const unsigned nthr = numThreads();
		if (0 == nthr) { return; }

		init_barriers();

		for (unsigned t = 0; t < nthr; ++t) {
			bmThreads[t] = BenchmarkThread {t, this};
			const int rc = pthread_create(&pthreadIDs[t], NULL, threadMain, &bmThreads[t]);
			if (rc) { throw system_error(rc, generic_category(), strerror(rc)); }
			setaffinity_linux(t, pthreadIDs[t]);
		}
	}

	void ThreadedBenchmark::tearDown() {
		const unsigned nthr = numThreads();
		if (0 == nthr) { return; }

		for (unsigned t = 0; t < nthr; ++t)
			pthread_join(pthreadIDs[t], NULL);

		destroy_barriers();
	}

	void ThreadedBenchmark::next() {
		threadRange.next();
	}
	
	bool ThreadedBenchmark::atMin() const {
		return threadRange.atMin();
	}
	
	bool ThreadedBenchmark::atMax() const {
		return threadRange.atMax();
	}
	
	void ThreadedBenchmark::gotoBegin() {
		threadRange.gotoBegin();
	}
	
	void ThreadedBenchmark::gotoEnd() {
		threadRange.gotoEnd();
	}
	
	bool ThreadedBenchmark::equals(const RangeInterface & ri) const {
		const ThreadedBenchmark & tmp = dynamic_cast<const ThreadedBenchmark &>(ri);
		return threadRange.equals(tmp.threadRange);
	}

	ostream & ThreadedBenchmark::toOStream(ostream & os) const {
		os << "ThreadedBenchmark: ";
		return threadRange.toOStream(os);
	}

	unsigned ThreadedBenchmark::minThreads() const {
		return threadRange.min;
	}

	unsigned ThreadedBenchmark::maxThreads() const {
		return threadRange.max;
	}

	unsigned ThreadedBenchmark::numThreads() const {
		return threadRange.getValue();
	}

	void ThreadedBenchmark::runThreads() {
		if (numThreads() > 0)
			pthread_barrier_wait(&runThreads_b);
	}

	void ThreadedBenchmark::runThread(unsigned threadNum) {
		//for (;;) {
			//cerr << "thread " << threadNum << " waiting" << endl;
			//pthread_barrier_wait(&runThreads_b);
			//if (stopThreads) { return; }

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
		//}
	}

	// placeholders: no pure virtual methods to allow children to override no
	// more methods than they need
	void ThreadedBenchmark::init(unsigned) {}
	void ThreadedBenchmark::ready(unsigned) {}
	void ThreadedBenchmark::set(unsigned) {}
	void ThreadedBenchmark::go(unsigned) {}
	void ThreadedBenchmark::finish(unsigned) {}
}
