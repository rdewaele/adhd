#include <iostream>
#include <type_traits>
#include <stdexcept>

#include "arraywalk.hpp"

using namespace std;
using namespace arraywalk;

static void report(size_t idx_size, uint64_t cycles, uint64_t reads) {
	cout << "cycles: " << cycles << endl;
	cout << "total reads: " << reads << " (" << reads * idx_size << " bytes)" << endl;
	cout << "~cycles per read: "
		<< static_cast<double>(cycles) / static_cast<double>(reads) << endl << endl;
}

template <typename INDEX_T>
static void run_test(size_t arraysize, uint32_t loops) {
	uint64_t cycles = 0;
	uint64_t reads = 0;
	try {
		ArrayWalk<INDEX_T> test(arraysize, 128, RANDOM);
		test.timedwalk_loc1(loops, cycles, reads);
		report(sizeof(INDEX_T), cycles, reads);
	} catch (length_error & e) {}
}

int main() {
	size_t arraysize;
	for (arraysize = 1ul << 7; arraysize <= 1ul << 26; arraysize *= 2) {
		constexpr uint32_t loops = 1 << 10;

		run_test<uint8_t>(arraysize, loops);
		run_test<uint16_t>(arraysize, loops);
		run_test<uint32_t>(arraysize, loops);
		run_test<uint64_t>(arraysize, loops);
		run_test<__uint128_t>(arraysize, loops);
	}
	return 0;
}
