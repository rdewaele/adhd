#include <cstdint>
#include <iostream>
#include <random>
#include <cassert>

using namespace std;

using IDX_T = uint64_t;

constexpr static const IDX_T LENGTH = 100;
constexpr static const IDX_T ITERATIONS = 100;

//#define RNG_UNIFORM
#define RNG_BINOMIAL

//default_random_engine rng;
random_device rng;

#ifdef RNG_UNIFORM
static IDX_T randomIndex(IDX_T min) {
	uniform_int_distribution<IDX_T> dis(min, LENGTH - 1);
	return dis(rng);
}
#endif

#ifdef RNG_BINOMIAL
static IDX_T randomIndex(IDX_T min) {
	binomial_distribution<IDX_T> dis(LENGTH - 1 - min, 0.9);
	return dis(rng) + min;
}
#endif

bool isFullCycle(const IDX_T * const array) {
	IDX_T i, idx;
	bool visited[LENGTH];

	for (idx = 0; idx < LENGTH; ++idx)
		visited[idx] = false;

	for (i = 0, idx = 0; i < LENGTH; ++i, idx = array[idx])
		visited[idx] = true;

	for (idx = 0; idx < LENGTH; ++idx)
		if (!visited[idx])
			return false;

	return true;
}

int main() {
	for (IDX_T iter = 0; iter < ITERATIONS; ++ iter) {
		IDX_T array[LENGTH];
		IDX_T idx, rnd, swp;

		// init
		for (idx = 0; idx < LENGTH; ++idx)
			array[idx] = idx;

		// shuffle
		for (idx = 0; idx < LENGTH - 1; ++idx) {
			rnd = randomIndex(idx + 1);
			swp = array[idx];
			array[idx] = array[rnd];
			array[rnd] = swp;
		}

		// check
		assert(isFullCycle(array));

		// print
		for (idx = 0; idx < LENGTH-1; ++idx)
			cout << array[idx] << ",";
		cout << array[idx] << endl;
	}

	return 0;
}
