#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "arraywalk.hpp"
#include "prettyprint.hpp"
#include "timings.hpp"

using namespace std;
using namespace arraywalk;
using namespace prettyprint;

static void report(size_t idx_size, Timings & timings) {
	cout << timings.length << " elements x " << Bytes{idx_size} << " = "
		<< Bytes{timings.length * idx_size} << " | " << timings.istreams
		<< " instruction streams" << endl;
	cout << "cycles: " << timings.cycles << " | ";
	cout << "reads: " << timings.reads << " (" << Bytes{timings.reads * idx_size} << ")" << endl;
	cout << "~cycles per read: "
		<< (double) timings.cycles / (double) timings.reads << endl << endl;
}

// may throw domain_error when requested alignment is not a power of two
template <typename INDEX_T>
static void run_test() {
	try {
		auto test = ArrayWalk<INDEX_T>();
		test.run([] (Timings timings) { report(sizeof(INDEX_T), timings); });
	}
	catch (const length_error & e) { /* deliberately ignored */ }
}

int main() {
	run_test<uint8_t>();
	run_test<uint16_t>();
	run_test<uint32_t>();
	run_test<uint64_t>();
	run_test<__uint128_t>();

	return 0;
}
