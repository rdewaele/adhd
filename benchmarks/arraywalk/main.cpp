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

// may throw domain_error when requested alignment is not a power of two
template <typename INDEX_T>
static void run_test() {
	try {
		auto test = ArrayWalk<INDEX_T>();
		test.run([] (const Timings & timings) {
				cout
				<< "--- CSV ------------------------------------------" << endl
				<< timings.asCSV() << endl
				<< "--- HUMAN ----------------------------------------" << endl
				<< timings.asHuman() << endl;
				});
	}
	catch (const length_error &) { /* deliberately ignored */ }
}

int main() {
	run_test<uint8_t>();
	run_test<uint16_t>();
	run_test<uint32_t>();
	run_test<uint64_t>();
	run_test<__uint128_t>();

	return 0;
}
