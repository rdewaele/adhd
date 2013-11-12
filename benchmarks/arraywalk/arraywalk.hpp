#pragma once

#include <cstdint>
#include <random>
#include <type_traits>

#define TIMEDWALK_LOC_DEC(NUM) \
	INDEX_T timedwalk_loc##NUM(uint_fast32_t, uint64_t &, uint64_t &)

namespace arraywalk {

	enum pattern { RANDOM, INCREASING, DECREASING };

	template <typename INDEX_T>
		class ArrayWalk {
			public:
				ArrayWalk();
				ArrayWalk(size_t size, size_t align, pattern ptrn);
				~ArrayWalk();

				void init(size_t size, size_t align, pattern ptrn);

				size_t getLength();

				void reset(enum pattern);

				INDEX_T timedwalk_loc(unsigned, uint_fast32_t, uint64_t &, uint64_t &);
				INDEX_T timedwalk_vec(uint_fast32_t steps, uint64_t & cycles, uint64_t & reads);

			private:
				size_t length;
				INDEX_T * arraymem;
				INDEX_T * array;

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
