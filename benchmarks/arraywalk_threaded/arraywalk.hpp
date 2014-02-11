#pragma once

#include "../benchmark.hpp"
#include "config.hpp"
#include "timings.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>

#define TIMEDWALK_LOC_DEC(NUM) \
	INDEX_T timedwalk_loc##NUM(uint_fast32_t, uint64_t &, uint64_t &)

namespace arraywalk {

	template <typename INDEX_T>
		class ArrayWalk: public adhd::ThreadedBenchmark {
			public:
				ArrayWalk(const Config & cfg = Config());
				~ArrayWalk();

				virtual ArrayWalk * clone() const final override;

				virtual void init(unsigned threadNum) final override;
				virtual void ready(unsigned threadNum) final override;
				virtual void set(unsigned threadNum) final override;
				virtual void go(unsigned threadNum) final override;
				virtual void finish(unsigned threadNum) final override;

			private:
				Config config;
				size_t length;
				INDEX_T * arraymem;
				INDEX_T * array;

				INDEX_T timedwalk_loc(unsigned locs, uint_fast32_t MiB,
						uint64_t & cycles, uint64_t & reads);
				INDEX_T timedwalk_vec(uint_fast32_t MiB,
						uint64_t & cycles, uint64_t & reads);

				void random();
				void increasing();
				void decreasing();

				bool isFullCycle();

				std::default_random_engine rng;
				INDEX_T randomIndex(INDEX_T minimum);

				TIMEDWALK_LOC_DEC(1);
				TIMEDWALK_LOC_DEC(2);
				TIMEDWALK_LOC_DEC(3);
				TIMEDWALK_LOC_DEC(4);
				TIMEDWALK_LOC_DEC(5);
				TIMEDWALK_LOC_DEC(6);
				TIMEDWALK_LOC_DEC(7);
				TIMEDWALK_LOC_DEC(8);
				TIMEDWALK_LOC_DEC(9);
				TIMEDWALK_LOC_DEC(10);
				TIMEDWALK_LOC_DEC(11);
				TIMEDWALK_LOC_DEC(12);
				TIMEDWALK_LOC_DEC(13);
				TIMEDWALK_LOC_DEC(14);
				TIMEDWALK_LOC_DEC(15);
				TIMEDWALK_LOC_DEC(16);
				TIMEDWALK_LOC_DEC(17);
				TIMEDWALK_LOC_DEC(18);
				TIMEDWALK_LOC_DEC(19);
				TIMEDWALK_LOC_DEC(20);
		};
}
