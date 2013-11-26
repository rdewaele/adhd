#include <cassert>
#include <cstddef>
#include <exception>
#include <iostream>
#include <random>
#include <stdexcept>
#include <type_traits>

#include "reduction.hpp"
#include "../benchmark.hpp"
#include "../rdtsc.h"
#include "timings.hpp"
#include "util.hpp"

using namespace std;

namespace reduction {
	// The tested range of memory addresses will be smaller than
	// indicated/requested by the size parameter if INDEX_T's range is
	// insufficiently large.
	static const char NOT_INDEXABLE[] =
		"Walking array is too long or index type range is too small.";

	// Arrays with three or less elements can not encode a random access pattern
	// (as opposed to a sequential pattern), rendering the random tests invalid.
	// (Additionally, tests with arrays of smaller size than a cache line seem
	//  questionable.)
	static const char NEED_FOUR_ELEMENTS[] =
		"Walking array is too short to generate random access patterns.";

	// Memory alignment must be a power of two.
	static const char NOT_POW2_ALIGN[] =
		"Requested memory alignment for the walking array is not a power of two.";

	// Default-constructed class does not have an array to walk, and walking it
	// is therefore impossible.
	static const char NOT_INITIALIZED[] =
		"Default-constructed walking array was not initialized.";

	adhd::Benchmark * ReductionFactory::makeBenchmark(const adhd::Config & cfg) {
		// TODO: use actual config passed here, instead of using default config
		return new Reduction<uint64_t>(Config());
	}

	template <typename INDEX_T>
	Reduction<INDEX_T>::Reduction(const Config & _config):
		config(_config),
		length(0),
		arraymem(NULL),
		array(NULL)
	{}

	template <typename INDEX_T>
	Reduction<INDEX_T>::~Reduction()
	{
		delete[] arraymem;
	}

	template <typename INDEX_T>
	void Reduction<INDEX_T>::run(adhd::timing_cb tcb)
	{
		for (size_t size = config.size_min;
				size <= config.size_max;
				size = size * config.size_mul + config.size_inc)
		{
			length = size / sizeof(INDEX_T);

			/* icpc warns about implicit conversion, which is rather odd when doing
			 * an explicit conversion. Furthermore, using a function- or c-style cast
			 * also suppresses the warning, but these should resolve to one of the
			 * xxx_cast<>() forms in C++11 according to the standard, and in this
			 * specific case be equivalent to the static_cast written below. Lastly,
			 * clang nor gcc produce a warning. Last checked with:
			 * icpc (ICC) 14.0.1 20131008, clang version 3.3 (tags/RELEASE_33/final)
			 * and g++ (GCC) 4.8.2 */
#ifdef __INTEL_COMPILER
#pragma warning(push)
#pragma warning(disable:1682)
#endif
			// numeric_limits might not be specialized for non-standard types
			// (e.g. __int128 with icc)
			INDEX_T rep_length = static_cast<INDEX_T>(length);
#ifdef __INTEL_COMPILER
#pragma warning(pop)
#endif
			if (rep_length != length)
				throw length_error(NOT_INDEXABLE);

			if (length < 4)
				throw length_error(NEED_FOUR_ELEMENTS);

			if (!util::isPowerOfTwo<size_t>(config.align))
				throw domain_error(NOT_POW2_ALIGN);

			// allocate with overhead to cater for later alignment
			// arraymem must be NULL or previously allocated by this function
			const size_t overhead = 1 + config.align / sizeof(INDEX_T);
			delete[] arraymem;
			arraymem = new INDEX_T[length + overhead];
			const uintptr_t aligned =
				(reinterpret_cast<uintptr_t>(arraymem) + config.align) & (~(config.align - 1));
			array = reinterpret_cast<INDEX_T *>(aligned);

			// init with ones
			INDEX_T idx;
			for (idx = 0; idx < length; ++idx)
				array[idx] = static_cast<INDEX_T>(1);

			uint64_t cycles;
			uint64_t reads;
			// loop through data-invariant tests
			for (unsigned istream = config.istream_min;
					istream <= config.istream_max;
					++istream)
			{
				cout << "total: " << timedreduce_loc(istream, config.MiB, cycles, reads) << endl;
				tcb(Timings(TimingData { cycles, reads, length, sizeof(INDEX_T), istream }));
			}
		}
	}

#include "reduction_loc.ii"
#include "reduction_vec.ii"

	//template class Reduction<uint8_t>;
	//template class Reduction<uint16_t>;
	//template class Reduction<uint32_t>;
	template class Reduction<uint64_t>;
	//template class Reduction<__uint128_t>;
}
