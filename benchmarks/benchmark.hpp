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
			// BenchmarkInterface
			virtual void run() = 0;

			// RangeInterface
			// overload return type to enable range-based loops for
			// BenchmarkInterfaces specifically
			virtual BenchmarkInterface * clone() const = 0;
	};

	template <typename BIC>
		void runBenchmark(BIC & bi) {
			for (auto & tmp: bi)
				tmp.run();
		}

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
			virtual std::ostream & toOStream(std::ostream & os) const override;

			bool operator==(const SingleBenchmark &) const;
			bool operator!=(const SingleBenchmark &) const;

		private:
			bool reset;
	};

	class ThreadedBenchmark: public BenchmarkInterface {
		public:
			ThreadedBenchmark(unsigned minThreads, unsigned maxThreads);
			ThreadedBenchmark(const ThreadedBenchmark &) = delete;
			virtual ~ThreadedBenchmark();

			// RangeInterface
			virtual void next() override;
			virtual bool atMin() const override;
			virtual bool atMax() const override;
			virtual void gotoBegin() override;
			virtual void gotoEnd() override;
			virtual std::ostream & toOStream(std::ostream & os) const override;

			bool operator==(const ThreadedBenchmark &) const;
			bool operator!=(const ThreadedBenchmark &) const;

			// BenchmarkInterface
			virtual void run() override;

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

			void spawnThreads();
			void joinThreads();

			std::vector<pthread_t> pthreadIDs;
			std::vector<BenchmarkThread> bmThreads;

			Range<unsigned> threadRange;

			pthread_barrier_t runThreads_entry_b;
			pthread_barrier_t runThreads_exit_b;
			bool stopThreads;
			bool threadsRunning;

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
