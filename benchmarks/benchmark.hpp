#pragma once

#define INT_LOCK

#include "config.hpp"
#include "prettyprint.hpp"
#include "timings.hpp"

#include <atomic>
#include <functional>
#include <iostream>
#include <iterator>
#include <string>

namespace adhd {

	class Benchmark;
	class SimpleBenchmark;
	class ThreadedBenchmark;

	class BenchmarkFactory {
		public:
			virtual Benchmark * makeBenchmark(const Config & cfg) const = 0;
			virtual ~BenchmarkFactory() = default;
	};

	class Benchmark {
		public:
			Benchmark(const Config & cfg);
			virtual ~Benchmark();
			void run(timing_cb tcb);
			virtual void runProcess(timing_cb tcb) = 0;
		protected:
			const Config * const config;
	};

	class SimpleBenchmark: public Benchmark {
		public:
			SimpleBenchmark(const Config & cfg);
			virtual void runProcess(timing_cb tcb) final override;
			virtual void runBare(timing_cb tcb) = 0;
			virtual ~SimpleBenchmark() {};
	};

	class ThreadedBenchmark: public Benchmark {
		protected:
			class Context;
			class ContextFactory;

		public:
			ThreadedBenchmark(const Config & cfg, const ContextFactory & ctxFac = ContextFactory());
			virtual void runProcess(timing_cb tcb) final override;
			virtual ~ThreadedBenchmark();

		protected:
			enum class Phase { INIT, READY, SET, GO, FINISH };

			virtual void runPhase(Phase p, unsigned threadNum) = 0;

			void reportTimings(unsigned threadNum, const Timings & timings);

			class ContextFactory {
				public:
					virtual Context * makeContext(unsigned numThreads) const;
					virtual ~ContextFactory() = default;
			};

			class Context {
				public:
					Context(const unsigned numThreads);
					~Context();

					inline unsigned getNumThreads() { return numThreads; }

					void storeTimings(const unsigned threadNum, const Timings & timings);
					void deleteTimings();
					void deleteTiming(const unsigned threadNum);


				private:
					friend class ThreadedBenchmark;
					unsigned numThreads;
					Timings ** timings;

					pthread_barrier_t init;
					pthread_barrier_t ready;
					pthread_barrier_t set;
					pthread_barrier_t go;
					pthread_barrier_t finish;

					std::atomic_flag cont;
			};

		private:
			void * runThread(unsigned threadNum);

			void init(unsigned threadNum);
			void ready(unsigned threadNum);
			void set(unsigned threadNum);
			void go(unsigned threadNum);
			void finish(unsigned threadNum);

			friend struct BenchmarkThread;

			Context * context;
			ContextFactory * contextFactory;
			const unsigned minThreads;
			const unsigned maxThreads;
			const pthread_t self;
			pthread_t * allThreads;
#if defined FLAG_LOCK
			std::atomic_flag spin_go;
#elif defined BOOL_LOCK
#elif defined INT_LOCK
			std::atomic_int spin_go;
#endif
	};
}
