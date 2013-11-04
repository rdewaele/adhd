#include <cassert>
#include <exception>
#include <iostream>
#include <random>
#include <type_traits>

#include "arraywalk.hpp"
#include "rdtsc.h"

using namespace std;

namespace arraywalk {
	template <typename INDEX_T>
		ArrayWalk<INDEX_T>::ArrayWalk(INDEX_T _length, enum pattern pattern):
			length(_length),
			array(reinterpret_cast<INDEX_T *>(new INDEX_T[_length]))
	{
		cout << "array elements: " << static_cast<uint64_t>(_length) << endl;
		cout << "element size: " << sizeof(INDEX_T) << endl;
		cout << "total array size: " << static_cast<uint64_t>(sizeof(INDEX_T) * _length) << endl;
		switch (pattern) {
			case RANDOM: random(); break;
			case INCREASING: increasing(); break;
			case DECREASING: decreasing(); break;
		}
	}

	template <typename INDEX_T>
		ArrayWalk<INDEX_T>::~ArrayWalk()
		{
			delete[] array;
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