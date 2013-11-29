#pragma once

#include "../benchmark.hpp"
#include "config.hpp"
#include "timings.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>

#define TIMEDREDUCE_LOC_DEC(NUM) \
	INDEX_T timedreduce_loc##NUM(uint_fast32_t, uint64_t &, uint64_t &)

namespace reduction {

	class ReductionFactory: public adhd::BenchmarkFactory {
		public:
			virtual adhd::Benchmark * makeBenchmark(const adhd::Config & cfg) const override;
	};

	template <typename INDEX_T>
		class Reduction: public adhd::Benchmark {
			public:
				Reduction(const Config & cfg = Config());
				~Reduction();

				virtual void run(adhd::timing_cb tcb) override;

			private:
				Config config;
				size_t length;
				INDEX_T * arraymem;
				INDEX_T * array;

				INDEX_T timedreduce_loc(unsigned locs, uint_fast32_t MiB,
						uint64_t & cycles, uint64_t & reads);
				INDEX_T timedreduce_vec(uint_fast32_t MiB,
						uint64_t & cycles, uint64_t & reads);

				TIMEDREDUCE_LOC_DEC(1);
				TIMEDREDUCE_LOC_DEC(2);
				TIMEDREDUCE_LOC_DEC(3);
				TIMEDREDUCE_LOC_DEC(4);
				TIMEDREDUCE_LOC_DEC(5);
				TIMEDREDUCE_LOC_DEC(6);
				TIMEDREDUCE_LOC_DEC(7);
				TIMEDREDUCE_LOC_DEC(8);
				TIMEDREDUCE_LOC_DEC(9);
				TIMEDREDUCE_LOC_DEC(10);
				TIMEDREDUCE_LOC_DEC(11);
				TIMEDREDUCE_LOC_DEC(12);
				TIMEDREDUCE_LOC_DEC(13);
				TIMEDREDUCE_LOC_DEC(14);
				TIMEDREDUCE_LOC_DEC(15);
				TIMEDREDUCE_LOC_DEC(16);
				TIMEDREDUCE_LOC_DEC(17);
				TIMEDREDUCE_LOC_DEC(18);
				TIMEDREDUCE_LOC_DEC(19);
				TIMEDREDUCE_LOC_DEC(20);
		};
}
