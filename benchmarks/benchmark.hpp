#pragma once

#include "config.hpp"
#include "range.hpp"
#include "timings.hpp"

#include <atomic>
#include <iterator>
#include <pthread.h>

namespace adhd {

	class BenchmarkInterface: public virtual RangeInterface {
		public:
			virtual void run(timing_cb tcb) = 0;
			virtual ~BenchmarkInterface() throw() = default;
	};

	// One-shot benchmark: no variants
	class SingleBenchmark: public BenchmarkInterface {
		public:
			SingleBenchmark();

			// RangeInterface
			virtual std::ostream & toOStream(std::ostream & os) const override;

			// BenchmarkInterface
			virtual void run(timing_cb) final override;

			// RangeInterface
			virtual void next() final override;
			virtual bool atMin() const final override;
			virtual bool atMax() const final override;
			virtual void gotoBegin() final override;
			virtual void gotoEnd() final override;
			virtual bool equals(const RangeInterface &) const final override;

		protected:
			virtual void runSingle(timing_cb) const = 0;

		private:
			bool reset;
	};

	class ThreadedBenchmark: public BenchmarkInterface {
		public:
			ThreadedBenchmark(unsigned minThreads, unsigned maxThreads);
			ThreadedBenchmark(const ThreadedBenchmark &);
			virtual ~ThreadedBenchmark() throw();

			// BenchmarkInterface
			virtual void run(timing_cb tcb) final override;

			// RangeInterface
			virtual void next() override;
			virtual bool atMin() const override;
			virtual bool atMax() const override;
			virtual void gotoBegin() override;
			virtual void gotoEnd() override;
			virtual bool equals(const RangeInterface &) const override;
			virtual std::ostream & toOStream(std::ostream & os) const override;

			unsigned minThreads() const;
			unsigned maxThreads() const;
			unsigned numThreads() const;

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
			void * runThread(unsigned threadNum);

			friend struct BenchmarkThread;

			Range<unsigned> threadRange;
			pthread_t * allThreads;

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
