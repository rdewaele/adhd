#include "benchmark.hpp"

#include <cstring>
#include <pthread.h>
#include <stdexcept>
#include <system_error>

#if defined FLAG_LOCK
#include <unistd.h>
#elif defined BOOL_LOCK
#include <unistd.h>
#elif defined INT_LOCK
#include <unistd.h>
#endif

using namespace std;

namespace adhd {

	static const char keyMinThreads[] = "minThreads";
	static const char keyMaxThreads[] = "maxThreads";

	/*
	 * Benchmark
	 */
	void Benchmark::run(timing_cb tcb) {
		runProcess(tcb);
	}


	/*
	 * SimpleBenchmark
	 */

	void SimpleBenchmark::runProcess(timing_cb tcb) {
		runBare(tcb);
	}


	/*
	 * ThreadedBenchmark
	 */

	// allow the plain old C function passed to pthread_create to invoke the
	// benchmark, supplying some extra thread-unique parameters as argument
	struct BenchmarkThread {
		unsigned threadNum;
		ThreadedBenchmark * benchmark;
		inline void * runThread() {
			return benchmark->runThread(threadNum);
		}
	};

	// main thread function that will be passed to pthread_create, as method
	// pointers can (should/must) not be used; qualifying the function as
	// extern "C" maximises portability
	extern "C" {
		static void * c_thread_main(BenchmarkThread * bt) {
			return bt->runThread();
		}
	}

	ThreadedBenchmark::ThreadedBenchmark(const Config & cfg):
		context(NULL),
		/* minThreads(cfg.get<unsigned>(keyMinThreads)),
		maxThreads(cfg.get<unsigned>(keyMaxThreads)) */
		minThreads(1),
		maxThreads(4),
		self(pthread_self()),
		allThreads(new pthread_t[maxThreads])
#if defined FLAG_LOCK
		,spin_go(ATOMIC_FLAG_INIT)
#elif defined BOOL_LOCK
#elif defined INT_LOCK
		,spin_go(0)
#endif
	{
		const auto min = minThreads;
		const auto max = maxThreads;
		for (unsigned t = min; t < max; ++t)
			allThreads[t] = self;
		// TODO config implementation
	}

	ThreadedBenchmark::~ThreadedBenchmark() {
		// XXX when an exception occurs, threads may be orphaned
		// however joining them here would very like result in a deadlock
		delete context;
		delete[] allThreads;
	}

	void ThreadedBenchmark::runProcess(timing_cb tcb) {
		const auto min = minThreads;
		const auto max = maxThreads;
		// range over threads
		for (unsigned num = min; num <= max; ++num) {
			// range over data sets
			//SharedData::Iterator dataIter(data);
			for (auto & a: *data) {}

			const auto tmain = reinterpret_cast<void * (*)(void *)>(c_thread_main);

			delete context;
			context = new ThreadContext(num);

			auto bts = new BenchmarkThread[num];

			for (unsigned t = 0; t < num; ++t) {
				bts[t] = BenchmarkThread {t, this};
				const int rc = pthread_create(allThreads + t, NULL, tmain, bts + t);
				if (rc)
					throw system_error(rc, generic_category(), strerror(rc));
			}

			for (unsigned t = 0; t < num; ++t) {
				pthread_join(allThreads[t], NULL);
				allThreads[t] = self;
			}

			delete[] bts;
		}
	}

	void ThreadedBenchmark::reportTimings(unsigned threadNum, const Timings & timings) {
		context->storeTimings(threadNum, timings);
	}

	void * ThreadedBenchmark::runThread(unsigned threadNum) {
		/*- barrier --------------------------------------------------------------*/
		const int isSerial_init = context->init();
		if (isSerial_init == PTHREAD_BARRIER_SERIAL_THREAD) {
#if defined FLAG_LOCK
			spin_go.test_and_set();
#elif defined BOOL_LOCK
#elif defined INT_LOCK
			spin_go = context->getNumThreads();
#endif
			setup(context->getNumThreads());
		}

		/*- barrier --------------------------------------------------------------*/
		context->ready();
		warmup();

		/*- barrier --------------------------------------------------------------*/
		context->set();
		// XXX this used to contain timing calls

		/*- barrier --------------------------------------------------------------*/
		const int isSerial_go = context->go();
#if defined FLAG_LOCK
		if (isSerial_init == PTHREAD_BARRIER_SERIAL_THREAD) {
			sleep(1);
			spin_go.clear();
		}
		else {
			while(spin_go.test_and_set());
			spin_go.clear();
		}
#elif defined BOOL_LOCK
#elif defined INT_LOCK
		spin_go--;
		while(spin_go);
#endif
		runBare(threadNum);

		/*- barrier --------------------------------------------------------------*/
		context->finish();
		// XXX all threads have finished - used to contain timing calls
		// XXX make an aggregate of all timings
	}


	/*
	 * ThreadContext
	 */

	ThreadedBenchmark::ThreadContext::ThreadContext(const unsigned _numThreads):
		numThreads(_numThreads),
		timings(new Timings * [_numThreads])
	{
		for (unsigned t = 0; t < numThreads; ++t)
			timings[t] = NULL;

		pthread_barrier_init(&b_init, NULL, numThreads);
		pthread_barrier_init(&b_ready, NULL, numThreads);
		pthread_barrier_init(&b_set, NULL, numThreads);
		pthread_barrier_init(&b_go, NULL, numThreads);
		pthread_barrier_init(&b_finish, NULL, numThreads);
	}

	ThreadedBenchmark::ThreadContext::~ThreadContext() {
		pthread_barrier_destroy(&b_init);
		pthread_barrier_destroy(&b_ready);
		pthread_barrier_destroy(&b_set);
		pthread_barrier_destroy(&b_go);
		pthread_barrier_destroy(&b_finish);

		delete[] timings;
	}

	void ThreadedBenchmark::ThreadContext::storeTimings(const unsigned threadNum, const Timings & threadTimings) {
		if (threadNum >= numThreads) {
			// TODO exception
			throw runtime_error("request to store timings for an out of bound thread index");
			return;
		}

		delete timings[threadNum];
		timings[threadNum] = threadTimings.clone();
	}

	void ThreadedBenchmark::ThreadContext::deleteTimings() {
		for (unsigned t = 0; t < numThreads; ++t) {
			delete timings[t];
			timings[t] = NULL;
		}
	}

	void ThreadedBenchmark::ThreadContext::deleteTiming(const unsigned threadNum) {
		if (threadNum >= numThreads)
			// TODO exception
			throw runtime_error("request to store timings for an out of bound thread index");
			return;

		delete timings[threadNum];
		timings[threadNum] = NULL;
	}
}
