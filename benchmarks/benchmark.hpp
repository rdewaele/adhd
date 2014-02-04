#pragma once

#include "config.hpp"
#include "range.hpp"
#include "timings.hpp"

#include <atomic>
#include <iterator>
#include <pthread.h>
#include <vector>

namespace adhd {

	class BenchmarkInterface: public virtual RangeInterface {
		public:
			void run() {
				do {
					setUp();
					tearDown();
				} while (!atMax() && (next(), true));
			}

		protected:
			virtual void setUp() = 0;
			virtual void tearDown() = 0;
	};

	// One-shot benchmark: no variants
	class SingleBenchmark: public BenchmarkInterface {
		public:
			SingleBenchmark();

			// RangeInterface
			virtual void next() final override;
			virtual bool atMin() const final override;
			virtual bool atMax() const final override;
			virtual void gotoBegin() final override;
			virtual void gotoEnd() final override;
			virtual bool equals(const RangeInterface &) const final override;

			virtual std::ostream & toOStream(std::ostream & os) const override;

			// BenchmarkInterface
			virtual void setUp() final override;
			virtual void tearDown() final override;

		protected:
			virtual void runSingle() = 0;

		private:
			bool reset;
	};

	class ThreadedBenchmark: public BenchmarkInterface {
		public:
			ThreadedBenchmark(unsigned minThreads, unsigned maxThreads);
			ThreadedBenchmark(const ThreadedBenchmark &) = delete;

			// RangeInterface
			virtual void next() override;
			virtual bool atMin() const override;
			virtual bool atMax() const override;
			virtual void gotoBegin() override;
			virtual void gotoEnd() override;
			virtual bool equals(const RangeInterface &) const override;
			virtual std::ostream & toOStream(std::ostream & os) const override;

			// ThreadedBenchmark
			unsigned minThreads() const;
			unsigned maxThreads() const;
			unsigned numThreads() const;

			// allow the plain old C function passed to pthread_create to invoke the
			// benchmark, supplying some extra thread-unique parameters as argument
			struct BenchmarkThread {
				unsigned threadNum;
				ThreadedBenchmark * benchmark;
				inline void runThread() { benchmark->runThread(threadNum); }
			};

		protected:
			// ThreadedBenchmark's threads will spawn once, and execute the
			// init/ready/set/go/finish methods when runThreads() is called.
			void runThreads();

			void setUp() override;
			void tearDown() override;

			// placeholders: no pure virtual methods to allow children to override no
			// more methods than they need
			virtual void init(unsigned threadNum);
			virtual void ready(unsigned threadNum);
			virtual void set(unsigned threadNum);
			virtual void go(unsigned threadNum);
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

			std::vector<pthread_t> pthreadIDs;
			std::vector<BenchmarkThread> bmThreads;

			Range<unsigned> threadRange;

			pthread_barrier_t runThreads_b;
			//bool stopThreads;

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
