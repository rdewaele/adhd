#pragma once

#define SPIN_LOCK

#include "prettyprint.hpp"

#include <atomic>
#include <functional>
#include <iostream>
#include <string>

namespace adhd {

	class Benchmark;
	class SimpleBenchmark;
	class ThreadedBenchmark;
	class Config;
	class Timings;

	typedef void timing_cb_t(const Timings &);
	typedef std::function<timing_cb_t> timing_cb;

	class BenchmarkFactory {
		public:
			virtual Benchmark * makeBenchmark(const Config & cfg) const = 0;
			virtual ~BenchmarkFactory() = default;
	};

	class Benchmark {
		public:
			virtual void run(timing_cb tcb) = 0;
			virtual ~Benchmark() {};
	};

	class SimpleBenchmark: public Benchmark {
		public:
			virtual void run(timing_cb tcb) final override;
			virtual void runBare(timing_cb tcb) = 0;
			virtual ~SimpleBenchmark() {};
	};

	class ThreadedBenchmark: public Benchmark {
		public:
			ThreadedBenchmark(const Config & cfg);
			virtual void run(timing_cb tcb) final override;
			virtual void setup(unsigned numThreads) = 0;
			virtual void warmup() = 0;
			virtual void runBare(unsigned threadNum) = 0;
			virtual ~ThreadedBenchmark();

		protected:
			void reportTimings(unsigned threadNum, const Timings & timings);

		private:
			friend struct BenchmarkThread;

			void runThreads(unsigned num);
			void * runThread(unsigned threadNum);

			struct ThreadContext {
				public:
					ThreadContext(const unsigned numThreads);
					~ThreadContext();

					inline unsigned getNumThreads() { return numThreads; }

					void storeTimings(const unsigned threadNum, const Timings & timings);
					void deleteTimings();
					void deleteTiming(const unsigned threadNum);

					inline int init()   { return pthread_barrier_wait(&b_init); }
					inline int ready()  { return pthread_barrier_wait(&b_ready); }
					inline int set()    { return pthread_barrier_wait(&b_set); }
					inline int go()     { return pthread_barrier_wait(&b_go); }
					inline int finish() { return pthread_barrier_wait(&b_finish); }

				private:
					unsigned numThreads;
					Timings ** timings;

					pthread_barrier_t b_init;
					pthread_barrier_t b_ready;
					pthread_barrier_t b_set;
					pthread_barrier_t b_go;
					pthread_barrier_t b_finish;
			} * context;
			const unsigned minThreads;
			const unsigned maxThreads;
			const pthread_t self;
			pthread_t * allThreads;
#if FLAG_LOCK
			std::atomic_flag spin_go;
#elif BOOL_LOCK
#endif
	};

	class Config {
		public:
			template <typename T>
				T get(const std::string & s) const;
	};

	class Timings: public prettyprint::CSV, public prettyprint::Human {
		public:
			virtual ~Timings() = default;
			virtual Timings * clone() const = 0;
			virtual std::ostream & formatHeader(std::ostream & out) const override = 0;
			virtual std::ostream & formatCSV(std::ostream & out) const override = 0;
			virtual std::ostream & formatHuman(std::ostream & out) const override = 0;
	};
}
