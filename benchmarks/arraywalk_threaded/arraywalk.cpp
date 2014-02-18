#include <cassert>
#include <cstddef>
#include <exception>
#include <iostream>
#include <random>
#include <stdexcept>
#include <type_traits>

#include "arraywalk.hpp"
#include "../benchmark.hpp"
#include "../rdtsc.h"
#include "timings.hpp"
#include "util.hpp"

using namespace adhd;
using namespace std;

namespace arraywalk {
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

	template <typename INDEX_T>
	ArrayWalk<INDEX_T>::ArrayWalk(const Config & cfg):
		ThreadedBenchmark(cfg.threads_min, cfg.threads_max),
		Config(cfg),
		length(0),
		arraymem(NULL),
		array(NULL)
	{}

	template <typename INDEX_T>
	ArrayWalk<INDEX_T>::~ArrayWalk()
	{
		delete[] arraymem;
	}

	template <typename INDEX_T>
		void ArrayWalk<INDEX_T>::init(unsigned /*threadNum*/) {
			length = Config::currentSize() / sizeof(INDEX_T);

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
			const size_t align = Config::currentAlign();
#ifdef __INTEL_COMPILER
#pragma warning(pop)
#endif
			if (rep_length != length)
				throw length_error(NOT_INDEXABLE);

			if (length < 4)
				throw length_error(NEED_FOUR_ELEMENTS);

			if (!util::isPowerOfTwo<size_t>(align))
				throw domain_error(NOT_POW2_ALIGN);

			// allocate with overhead to cater for later alignment
			// arraymem must be NULL or previously allocated by this function
			const size_t overhead = 1 + align / sizeof(INDEX_T);
			delete[] arraymem;
			arraymem = new INDEX_T[length + overhead];
			const uintptr_t aligned =
				(reinterpret_cast<uintptr_t>(arraymem) + align) & (~(align - 1));
			array = reinterpret_cast<INDEX_T *>(aligned);

			switch (Config::ptrn) {
				case Pattern::RANDOM: random(); break;
				case Pattern::INCREASING: increasing(); break;
				case Pattern::DECREASING: decreasing(); break;
			}


		}

	template <typename INDEX_T>
		void ArrayWalk<INDEX_T>::ready(unsigned /*threadNum*/) {
		}

	template <typename INDEX_T>
		void ArrayWalk<INDEX_T>::set(unsigned /*threadNum*/) {
		}

	template <typename INDEX_T>
	void ArrayWalk<INDEX_T>::go(unsigned threadNum)
	{
		uint64_t cycles;
		uint64_t reads;
		const unsigned istream = Config::currentIStream();

		go_wait_start();
		timedwalk_loc(istream, Config::readMiB, cycles, reads);
		go_wait_end();
		timing_callback(Timings(TimingData {
					numThreads(), threadNum,
					cycles, reads, length, sizeof(INDEX_T), istream, currentAlign()
					}));
	}

	template <typename INDEX_T>
		void ArrayWalk<INDEX_T>::finish(unsigned /*threadNum*/) {
		}

	template <typename INDEX_T>
		ArrayWalk<INDEX_T> * ArrayWalk<INDEX_T>::clone() const {
			return new ArrayWalk<INDEX_T>(static_cast<const Config &>(*this));
		}

	template <typename INDEX_T>
		void ArrayWalk<INDEX_T>::next() {
			Config::next();
			if (Config::atMin())
				ThreadedBenchmark::next();
		}

	template <typename INDEX_T>
		bool ArrayWalk<INDEX_T>::atMin() const {
			return ThreadedBenchmark::atMin() && Config::atMin();
		}

	template <typename INDEX_T>
		bool ArrayWalk<INDEX_T>::atMax() const {
			return ThreadedBenchmark::atMax() && Config::atMax();
		}

	template <typename INDEX_T>
		void ArrayWalk<INDEX_T>::gotoBegin() {
			ThreadedBenchmark::gotoBegin();
			Config::gotoBegin();
		}

	template <typename INDEX_T>
		void ArrayWalk<INDEX_T>::gotoEnd() {
			ThreadedBenchmark::gotoEnd();
			Config::gotoEnd();
		}

	template <typename INDEX_T>
		bool ArrayWalk<INDEX_T>::operator==(const ArrayWalk<INDEX_T> & rhs) const {
			return static_cast<const ThreadedBenchmark &>(*this) == rhs
				&& static_cast<const Config &>(*this) == rhs;
		}

	template <typename INDEX_T>
		bool ArrayWalk<INDEX_T>::operator!=(const ArrayWalk<INDEX_T> & rhs) const {
			return static_cast<const ThreadedBenchmark &>(*this) != rhs
				|| static_cast<const Config &>(*this) != rhs;
		}

	// the random library does not recognise __uint128_t as an integral type: specialize
	template <>
	__uint128_t ArrayWalk<__uint128_t>::randomIndex(__uint128_t minimum)
	{
		const uint64_t min = static_cast<uint64_t>(minimum);
		const uint64_t max = static_cast<uint64_t>(length - 1);
		uniform_int_distribution<uint64_t> dis(min, max);
		return dis(rng);
	}

	template <typename INDEX_T>
	INDEX_T ArrayWalk<INDEX_T>::randomIndex(INDEX_T minimum)
	{
		/* see disable:1682 above */
#ifdef __INTEL_COMPILER
#pragma warning(push)
#pragma warning(disable:1682)
#endif
		const INDEX_T maximum = static_cast<INDEX_T>(length - 1);
#ifdef __INTEL_COMPILER
#pragma warning(pop)
#endif
		uniform_int_distribution<INDEX_T> dis(minimum, maximum);
		return dis(rng);
	}

	template <typename INDEX_T>
	void ArrayWalk<INDEX_T>::random()
	{
		if (0 == length)
			return;

		// initialization step encodes index as values
		for (INDEX_T idx = 0; idx < length; ++idx)
			array[idx] = idx;

		// shuffle the array
		// idx goes up to the penultimate element because element at idx is
		// swapped with an element at > idx
		INDEX_T rnd;
		INDEX_T swap;
		for (INDEX_T idx = 0; idx < length - 1; ++idx) {
			rnd = randomIndex(static_cast<INDEX_T>(idx + 1));
			swap = array[idx];
			array[idx] = array[rnd];
			array[rnd] = swap;
		}
	}

	template <typename INDEX_T>
	void ArrayWalk<INDEX_T>::increasing()
	{
		INDEX_T idx;
		for (idx = 0; idx < length - 1; ++idx)
			array[idx] = static_cast<INDEX_T>(idx + 1);
		array[idx] = 0;
	}

	template <typename INDEX_T>
	void ArrayWalk<INDEX_T>::decreasing()
	{
		/* see disable:1682 above */
#ifdef __INTEL_COMPILER
#pragma warning(push)
#pragma warning(disable:1682)
#endif
		const INDEX_T maximum = static_cast<INDEX_T>(length - 1);
#ifdef __INTEL_COMPILER
#pragma warning(pop)
#endif
		array[0] = maximum;
		for (INDEX_T idx = 1; idx < length; ++idx)
			array[idx] = static_cast<INDEX_T>(idx - 1);
	}

	template <typename INDEX_T>
	bool ArrayWalk<INDEX_T>::isFullCycle()
	{
		INDEX_T i, idx;
		bool * visited = new bool[length];
		bool allVisited = true;

		for (idx = 0; idx < length; ++idx)
			visited[idx] = false;

		for (i = 0, idx = 0; i < length; ++i, idx = array[idx])
			visited[idx] = true;

		for (idx = 0; idx < length; ++idx)
			if (!visited[idx]) {
				allVisited = false;
				break;
			}

		delete[] visited;
		assert(allVisited); // crash in debug builds
		return allVisited;
	}

#include "arraywalk_loc.ii"
#include "arraywalk_vec.ii"

	//template class ArrayWalk<uint8_t>;
	//template class ArrayWalk<uint16_t>;
	//template class ArrayWalk<uint32_t>;
	template class ArrayWalk<uint64_t>;
	//template class ArrayWalk<__uint128_t>;
}
