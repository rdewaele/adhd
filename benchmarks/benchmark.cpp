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

	Benchmark::Benchmark(const Config & cfg):
		config(cfg.clone())
	{}

	Benchmark::~Benchmark() {
		delete config;
	}

	void Benchmark::run(timing_cb tcb) {
		runProcess(tcb);
	}


	/*
	 * SimpleBenchmark
	 */

	SimpleBenchmark::SimpleBenchmark(const Config & cfg):
		Benchmark(cfg)
	{}

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

	ThreadedBenchmark::ThreadedBenchmark(const Config & cfg, const ContextFactory & ctxFac):
		Benchmark(cfg),
		context(NULL),
		contextFactory(new ContextFactory(ctxFac)),
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
		delete contextFactory;
		delete[] allThreads;
	}

	void ThreadedBenchmark::runProcess(timing_cb tcb) {
		const auto min = minThreads;
		const auto max = maxThreads;
		// range over threads
		for (unsigned num = min; num <= max; ++num) {
			// range over configs
			for (auto & cfg: *config) {

				const auto tmain = reinterpret_cast<void * (*)(void *)>(c_thread_main);

				delete context;
				// TODO: make thread-specific config, and change makeContext to accept a config state
				context = contextFactory->makeContext(num);

				auto bts = new BenchmarkThread[num];

				for (unsigned t = 0; t < num; ++t) {
					bts[t] = BenchmarkThread {t, this};
					const int rc = pthread_create(allThreads + t, NULL, tmain, bts + t);
					if (rc)
						throw system_error(rc, generic_category(), strerror(rc));
				}

				cerr << "joining threads" << endl;
				for (unsigned t = 0; t < num; ++t) {
					pthread_join(allThreads[t], NULL);
					allThreads[t] = self;
				}

				delete[] bts;
			}
		}
	}

	void ThreadedBenchmark::reportTimings(unsigned threadNum, const Timings & timings) {
		context->storeTimings(threadNum, timings);
	}

	void ThreadedBenchmark::init(unsigned threadNum) {
		const int isSerial = pthread_barrier_wait(&context->init);
		if (PTHREAD_BARRIER_SERIAL_THREAD == isSerial) {
#if defined FLAG_LOCK
			spin_go.test_and_set();
#elif defined BOOL_LOCK
#elif defined INT_LOCK
			spin_go = context->getNumThreads();
#endif
			runPhase(Phase::INIT, threadNum);
		}
	}

	void ThreadedBenchmark::ready(unsigned threadNum) {
		pthread_barrier_wait(&context->ready);
		runPhase(Phase::READY, threadNum);
	}

	void ThreadedBenchmark::set(unsigned threadNum) {
		pthread_barrier_wait(&context->set);
		runPhase(Phase::SET, threadNum);
	}

	void ThreadedBenchmark::go(unsigned threadNum) {
		pthread_barrier_wait(&context->go);
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
		runPhase(Phase::GO, threadNum);
	}

	void ThreadedBenchmark::finish(unsigned threadNum) {
		pthread_barrier_wait(&context->finish);
		runPhase(Phase::FINISH, threadNum);
	}

	void * ThreadedBenchmark::runThread(unsigned threadNum) {
		init(threadNum);
		ready(threadNum);
		set(threadNum);
		go(threadNum);
		finish(threadNum);
	}

	/*
	 * Context
	 */

	ThreadedBenchmark::Context::Context(const unsigned _numThreads):
		numThreads(_numThreads),
		timings(new Timings * [_numThreads]),
		cont(true)
	{
		for (unsigned t = 0; t < numThreads; ++t)
			timings[t] = NULL;

		pthread_barrier_init(&init, NULL, numThreads);
		pthread_barrier_init(&ready, NULL, numThreads);
		pthread_barrier_init(&set, NULL, numThreads);
		pthread_barrier_init(&go, NULL, numThreads);
		pthread_barrier_init(&finish, NULL, numThreads);
	}

	ThreadedBenchmark::Context::~Context() {
		pthread_barrier_destroy(&init);
		pthread_barrier_destroy(&ready);
		pthread_barrier_destroy(&set);
		pthread_barrier_destroy(&go);
		pthread_barrier_destroy(&finish);

		delete[] timings;
	}

	void ThreadedBenchmark::Context::storeTimings(const unsigned threadNum, const Timings & threadTimings) {
		if (threadNum >= numThreads) {
			// TODO exception
			throw runtime_error("request to store timings for an out of bound thread index");
			return;
		}

		delete timings[threadNum];
		timings[threadNum] = threadTimings.clone();
	}

	void ThreadedBenchmark::Context::deleteTimings() {
		for (unsigned t = 0; t < numThreads; ++t) {
			delete timings[t];
			timings[t] = NULL;
		}
	}

	void ThreadedBenchmark::Context::deleteTiming(const unsigned threadNum) {
		if (threadNum >= numThreads)
			// TODO exception
			throw runtime_error("request to store timings for an out of bound thread index");
			return;

		delete timings[threadNum];
		timings[threadNum] = NULL;
	}

	/*
	 * ContextFactory
	 */

	ThreadedBenchmark::Context * ThreadedBenchmark::ContextFactory::makeContext(unsigned numThreads) const {
		return new Context(numThreads);
	}
}
