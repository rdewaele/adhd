#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "arraywalk.hpp"
#include "prettyprint.hpp"

using namespace std;
using namespace arraywalk;
using namespace prettyprint;

static void report(size_t idx_size, size_t length, uint64_t cycles, uint64_t reads) {
	cout << length << " elements x " << Bytes{idx_size} << " = "
		<< Bytes{length * idx_size} << endl;
	cout << "cycles: " << cycles << " | ";
	cout << "reads: " << reads << " (" << Bytes{reads * idx_size} << ")" << endl;
	cout << "~cycles per read: " << (double) cycles / (double) reads << endl << endl;
}

// may throw domain_error when requested alignment is not a power of two
template <typename INDEX_T>
static void run_test(size_t arraysize, uint32_t loops) {
	uint64_t cycles = 0;
	uint64_t reads = 0;
	try {
		ArrayWalk<INDEX_T> test(arraysize, 1 << 12, RANDOM);
		test.timedwalk_loc(1, loops, cycles, reads);
		report(sizeof(INDEX_T), test.getLength(), cycles, reads);
	}
	catch (length_error & e) { /* deliberately ignored */ }
}

int main() {
	constexpr uint32_t loops = 1 << 7;

	// standard use case test
	size_t arraysize;
	for (arraysize = 1ul << 7; arraysize <= 1ul << 10; arraysize *= 2) {
		run_test<uint8_t>(arraysize, loops);
		run_test<uint16_t>(arraysize, loops);
		run_test<uint32_t>(arraysize, loops);
		run_test<uint64_t>(arraysize, loops);
		run_test<__uint128_t>(arraysize, loops);
	}

	// default constructor test
	uint64_t cycles = 0;
	uint64_t reads = 0;
	auto test = ArrayWalk<uint64_t>();
	try {
		test.timedwalk_loc(1, loops, cycles, reads);
	}
	catch (length_error & e) { /* deliberately ignored */ }
	test.init(arraysize, 1 << 12, RANDOM);
	test.timedwalk_loc(1, loops, cycles, reads);
	report(sizeof(uint64_t), test.getLength(), cycles, reads);

	return 0;
}
