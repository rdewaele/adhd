#pragma once

#include "range.hpp"
#include "timings.hpp"

#include <atomic>
#include <iterator>
#include <pthread.h>
#include <vector>

namespace adhd {

	// XXX Benchmarks and ranges are very similar in the aspect that they share
	//     almost the same interface. However: ranges compose horizontally, but
	//     benchmarks do not, they compose vertically: run()'s scope should close
	//     over the different benchmarks it uses, and there should be only one
	//     run() method for a certain benchmark. We can use multiple inheritance
	//     to tie both the horizontal and the vertical composition, e.g. with
	//     Range(Set) implementing (most of) the benchmark's RangeInterface.
	class BenchmarkInterface: public virtual RangeInterface {
		public:
			virtual void run(timing_cb) = 0;

			// overload return type to enable range-based loops for
			// BenchmarkInterfaces specifically
			virtual BenchmarkInterface * clone() const = 0;
	};

	// for convenience :-)
	template <typename B>
		inline void runBenchmark(B & b, timing_cb tcb) {
			for (auto & i: b) { i.run(tcb); }
		}

	// One-shot benchmark: no variants
	class SingleBenchmark: public virtual BenchmarkInterface, public Range<unsigned> {
		public:
			SingleBenchmark();
			virtual SingleBenchmark * clone() const override = 0;
	};

	// Threaded benchmark: run a number of threads as simultaneously as possible
	class ThreadedBenchmark: public virtual BenchmarkInterface, public Range<unsigned> {
		public:
			ThreadedBenchmark(unsigned minThreads, unsigned maxThreads);
			ThreadedBenchmark(const ThreadedBenchmark &) = delete;
			virtual ~ThreadedBenchmark();

			// BenchmarkInterface
			virtual void run(timing_cb) override;
			virtual ThreadedBenchmark * clone() const = 0;

			// ThreadedBenchmark
			inline unsigned minThreads() const { return minValue; }
			inline unsigned maxThreads() const { return maxValue; }
			inline unsigned numThreads() const { return getValue(); }

			friend std::ostream & operator<<(std::ostream &, const ThreadedBenchmark &);

			// allow the plain old C function passed to pthread_create to invoke the
			// benchmark, supplying some extra thread-unique parameters as argument
			struct BenchmarkThread {
				unsigned threadNum;
				ThreadedBenchmark * benchmark;
				inline void runThread() { benchmark->runThread(threadNum); }
			};

		protected:
			// placeholders: no pure virtual methods to allow children to override no
			// more methods than they need, leaving only go() as abstract method
			virtual void init(unsigned threadNum);
			virtual void ready(unsigned threadNum);
			virtual void set(unsigned threadNum);
			virtual void go(unsigned threadNum) = 0;
			virtual void finish(unsigned threadNum);

			// synchronize in go() method before executing the actual benchmark code
			// e.g. loop variant setup depending on local state
			inline void go_wait_start() { --spin_go_wait; while(spin_go_wait); }

			// synchronize in go() method before executing operations depending on
			// local state that are not part of the benchmark itself, e.g. process
			// timing results
			inline void go_wait_end() { pthread_barrier_wait(&go_wait_b); }

		private:
			void runThread(unsigned threadNum);
			void init_barriers();
			void destroy_barriers();

			void spawnThreads();
			void joinThreads(bool force);

			std::vector<pthread_t> pthreadIDs;
			std::vector<BenchmarkThread> bmThreads;

			pthread_barrier_t runThreads_entry_b;
			pthread_barrier_t runThreads_exit_b;
			bool stopThreads;

			unsigned runningThreads;

			std::atomic_uint spin_go;
			std::atomic_uint spin_go_wait;

			pthread_barrier_t init_b;
			pthread_barrier_t ready_b;
			pthread_barrier_t set_b;
			pthread_barrier_t go_b;
			pthread_barrier_t go_wait_b;
			pthread_barrier_t finish_b;
	};
}
