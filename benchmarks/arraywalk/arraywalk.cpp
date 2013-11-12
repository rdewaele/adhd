#include <cassert>
#include <cstddef>
#include <exception>
#include <iostream>
#include <random>
#include <stdexcept>
#include <type_traits>

#include "arraywalk.hpp"
#include "prettyprint.hpp"
#include "rdtsc.h"
#include "util.hpp"

using namespace std;
using namespace prettyprint;

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
	ArrayWalk<INDEX_T>::ArrayWalk():
		length(0),
		arraymem(NULL),
		array(NULL)
	{}

	template <typename INDEX_T>
	ArrayWalk<INDEX_T>::ArrayWalk(size_t size, size_t align, pattern ptrn):
		ArrayWalk()
	{
		init(size, align, ptrn);
	}

	template <typename INDEX_T>
	ArrayWalk<INDEX_T>::~ArrayWalk()
	{
		delete[] arraymem;
	}

	template <typename INDEX_T>
	void ArrayWalk<INDEX_T>::init(size_t size, size_t align, pattern ptrn)
	{
		length = size / sizeof(INDEX_T);

		// numeric_limits might not be specialized for non-standard types
		// (e.g. __int128 with icc)
		INDEX_T rep_length = static_cast<INDEX_T>(length);
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
		const intptr_t aligned =
			(reinterpret_cast<intptr_t>(arraymem) + align) & (~(align - 1));
		array = reinterpret_cast<INDEX_T *>(aligned);

		switch (ptrn) {
			case RANDOM: random(); break;
			case INCREASING: increasing(); break;
			case DECREASING: decreasing(); break;
		}
	}

	template <typename INDEX_T>
	size_t ArrayWalk<INDEX_T>::getLength()
	{
		return length;
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
		uniform_int_distribution<INDEX_T> dis(minimum, static_cast<INDEX_T>(length - 1));
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
		array[0] = static_cast<INDEX_T>(length - 1);
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

	template class ArrayWalk<uint8_t>;
	template class ArrayWalk<uint16_t>;
	template class ArrayWalk<uint32_t>;
	template class ArrayWalk<uint64_t>;
	template class ArrayWalk<__uint128_t>;
}
