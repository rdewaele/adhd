#include "benchmark.hpp"

#include "rdtsc.h"

#include <iostream>

using namespace std;
using namespace adhd;

//
// TIMINGS
//

class PhonyTimings: public Timings {
	public:
		PhonyTimings(): start(0), stop(0) {}

		PhonyTimings(long long unsigned _start, long long unsigned _stop):
			start(_start), stop(_stop) {}

		virtual Timings * clone() const override {
			auto temp = new PhonyTimings();
			return temp;
		}

		virtual ostream & formatHeader(ostream & out) const override {
			return out << "PhonyHeader" << endl;
		}

		virtual ostream & formatCSV(ostream & out) const override {
			return out << "PhonyField" << endl;
		}

		virtual ostream & formatHuman(ostream & out) const override {
			return out
				<< "start:        " << start << endl
				<< "stop:         " << stop << endl
				<< "stop - start: " << stop - start << endl;
		}

	private:
		long long unsigned start;
		long long unsigned stop;
};

//
// SIMPLE
//

class TestSingle: public SingleBenchmark {
	public:
		virtual void runSingle(timing_cb tcb) const final override {
			long long unsigned start = rdtsc();
			cout << "I am Simple!" << endl;
			long long unsigned stop = rdtsc();
			tcb(PhonyTimings(start, stop));
		}

		virtual TestSingle * clone() const override { return new TestSingle(); }
};

//
// THREADED
//

class TestThreaded: public ThreadedBenchmark {
	public:
		TestThreaded(unsigned min, unsigned max, unsigned iters)
			: ThreadedBenchmark(min, max),
			iterations(Range<unsigned>(0, iters), Range<unsigned>(1, 2)),
			shared(nullptr),
			timings(nullptr),
			startCycles(nullptr)
	{}

		virtual TestThreaded * clone() const override { return new TestThreaded(*this); }	

		virtual void runPhase(Phase p, unsigned threadNum) final override {
			const char * str;
			long long unsigned start = 0;
			long long unsigned stop = 0;
			switch (p) {
				case Phase::INIT:
					{
						const unsigned nthr = numThreads();
						shared = new unsigned[nthr];
						timings = new PhonyTimings[nthr];
						startCycles = new long long unsigned[nthr];
						str = "init";
						break;
					}
				case Phase::READY:
					shared[threadNum] = threadNum;
					str = "ready";
					break;
				case Phase::SET:
					shared[threadNum] += 1;
					str = "set";
					break;
				case Phase::GO:
					spinlock();
					start = rdtsc();
					shared[threadNum] *= 2;
					stop = rdtsc();
					// TODO: insert a barrier first, then process timings in order to
					// minimize noise between threads (in threadedbenchmark implementation)
					timings[threadNum] = PhonyTimings(start, stop);
					startCycles[threadNum] = start;
					str = "go";
					break;
				case Phase::FINISH:
					delete[] shared;
					for (unsigned t = 0; t < numThreads(); ++t)
						cout << timings[t].asHuman();
					delete[] timings;
					long long unsigned min = startCycles[0];
					long long unsigned max = startCycles[0];
					for (unsigned t = 1; t < numThreads(); ++t) {
						const long long unsigned current = startCycles[t];
						if (min > current) { min = current; }
						if (max < current) { max = current; }
					}
					cout << "start spread: " << max - min << " cycles" << endl;
					delete[] startCycles;
					str = "finish";
					break;
			}
			cout << "thread " << threadNum << " - " << str << " @ " << start << endl;
		}

	private:
		// TODO: do something with these iterations :)
		RangeSet<unsigned, unsigned> iterations;
		unsigned * shared;
		PhonyTimings * timings;
		unsigned long long * startCycles;
};

int main() {
	timing_cb phonyCallback = [] (const Timings & timings) {
		cout << "header: "; timings.formatHeader(cout);
		cout << "csv: " << timings.asCSV();
		cout << "human: " << timings.asHuman() << endl;
	};
	{
		auto simple = TestSingle();
		simple.run(phonyCallback);
	}
	{
		auto * threaded = new TestThreaded(1, 3, 5);
		threaded->run(phonyCallback);
		delete threaded;
	}
	return 0;
}
