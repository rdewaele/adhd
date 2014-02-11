#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <type_traits>

#include "arraywalk.hpp"
#include "../benchmark.hpp"
#include "timings.hpp"

using namespace std;
using namespace arraywalk;

// may throw domain_error when requested alignment is not a power of two
template <typename INDEX_T>
static void run_test(ofstream & logfile, unsigned trial) {
	try {
		auto && aw = ArrayWalk<INDEX_T>(Config());
		runBenchmark(aw,
				[&logfile, &trial] (const adhd::Timings & timings) {
				logfile << trial << "," << timings.asCSV();
				cout << timings.asHuman() << endl;
				});
	}
	catch (const length_error &) { /* deliberately ignored */ }
}

int main(int argc, char * argv[]) {

	unsigned trials = 1;
	string filename = "arraywalk.log";

	// note: first argument is the actual executable's filename
	switch (argc) {
		case 3: // optional second argument determines the csv log filename
			{
				filename = string(argv[1]);
			}
		case 2: // optional first argument determines the number of trials to run
			{ 
				unsigned tmp;
				stringstream convert(argv[1]);
				if (convert >> tmp)
					trials = tmp;
			}
		case 1:
			break;
		default:
			cerr << "warning: third and subsequent arguments ignored" << endl;
	}

	cerr << "trials: " << trials << endl;

	ofstream logfile(filename);
	if (!logfile) {
		cerr << "failed to open CSV output file \"" << filename << "\"" << endl;
		return -1;
	}

	for (unsigned trial = 1; trial <= trials; ++trial) {
		cout << ">>> Trial " << trial << " >>>" << endl;
		//run_test<uint8_t>(logfile, trial);
		//run_test<uint16_t>(logfile, trial);
		//run_test<uint32_t>(logfile, trial);
		run_test<uint64_t>(logfile, trial);
		//run_test<__uint128_t>(logfile, trial);
	}

	return 0;
}
