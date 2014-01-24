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
			virtual ~BenchmarkInterface() = default;
	};

	class BenchmarkFactory {
		public:
			virtual BenchmarkInterface * makeBenchmark(const Config & cfg) const = 0;
			virtual ~BenchmarkFactory() = default;
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
			virtual ~ThreadedBenchmark();

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
			enum class Phase { INIT, READY, SET, GO, FINISH };

			virtual void runPhase(Phase p, unsigned threadNum) = 0;
			inline void spinlock() const { while(spin_go); }

			void reportTimings(unsigned threadNum, const Timings & timings);

		private:
			void * runThread(unsigned threadNum);

			void init(unsigned threadNum);
			void ready(unsigned threadNum);
			void set(unsigned threadNum);
			void go(unsigned threadNum);
			void finish(unsigned threadNum);

			friend struct BenchmarkThread;

			Range<unsigned> threadRange;
			pthread_t * allThreads;

			std::atomic_int spin_go;

			pthread_barrier_t init_b;
			pthread_barrier_t ready_b;
			pthread_barrier_t set_b;
			pthread_barrier_t go_b;
			pthread_barrier_t finish_b;
	};
}
