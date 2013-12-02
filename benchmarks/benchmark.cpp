#include "benchmark.hpp"

namespace adhd {

	void SimpleBenchmark::run(timing_cb tcb) {
		runBare(tcb);
	}

	void ThreadedBenchmark::run(timing_cb tcb) {
		const unsigned min = minThreads;
		const unsigned max = maxThreads;
		for (unsigned t = min; t <= max; ++t) {
			runBare(tcb);
		}
	}

}
