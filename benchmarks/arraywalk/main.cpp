#include <cstdint>
#include <iostream>
#include <type_traits>

#include "arraywalk.hpp"

using namespace std;
using namespace arraywalk;

static void
report(size_t idx_size, uint64_t cycles, uint64_t reads)
{
	cout << "cycles: " << cycles << endl;
	cout << "total reads: " << reads << " (" << reads * idx_size << " bytes)" << endl;
	cout << "~cycles per read: "
		<< static_cast<double>(cycles) / static_cast<double>(reads) << endl;
}

template <typename INDEX_T>
static void
run_test(size_t arraysize, uint32_t loops)
{
	uint64_t cycles = 0;
	uint64_t reads = 0;
	// C-style cast: icpc (but not gcc) issues an implicit conversion warning
	// when using static_cast (why?)
	INDEX_T length = (INDEX_T)(arraysize / sizeof(INDEX_T));
	ArrayWalk<INDEX_T> test(length, RANDOM);
	test.timedwalk_loc1(loops, cycles, reads);
	report(sizeof(INDEX_T), cycles, reads);
}

int
main()
{
	size_t arraysize;
	for (arraysize = 1ul << 7; arraysize <= 1ul << 26; arraysize *= 2) {
		constexpr uint32_t loops = 1 << 10;

		if (arraysize <= UINT8_MAX) {
			cout << ">>> uint8_t: ";
			run_test<uint8_t>(arraysize, loops);
		}

		if (arraysize < UINT16_MAX) {
			cout << ">>> uint16_t: ";
			run_test<uint16_t>(arraysize, loops);
		}

		if (arraysize <= UINT32_MAX) {
			cout << ">>> uint32_t: ";
			run_test<uint32_t>(arraysize, loops);
		}

		if (arraysize <= UINT64_MAX) {
			cout << ">>> uint64_t: ";
			run_test<uint64_t>(arraysize, loops);
		}

#ifdef __INTEL_COMPILER
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif /* __INTEL_COMPILER */
		if (arraysize < static_cast<__uint128_t>(-1)) {
#ifdef __INTEL_COMPILER
#else
#pragma GCC diagnostic pop
#endif /* __INTEL_COMPILER */
			cout << ">>> uint128_t: ";
			run_test<__uint128_t>(arraysize, loops);
		}
	}
	return 0;
}
