#include "benchmark.hpp"

#include "rdtsc.h"

#include <iostream>
#include <mutex>

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

		//virtual Timings * clone() const override {
		//	return new PhonyTimings(*this);
		//}

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

class SpreadTimings: public Timings {
	public:
		SpreadTimings(): SpreadTimings(0, 0, 0) {}

		void reset() {
			auto && ul = unique_lock<mutex>(mutexthis);
			threads = 0;
			min = max = 0;
			ul.unlock();
		}

		void addStamp(long long unsigned stamp) {
			auto && ul = unique_lock<mutex>(mutexthis);
			++threads;
			if (0 == (min + max))
				min = max = stamp;
			else if (stamp > max)
				max = stamp;
			else if (stamp < min)
				min = stamp;
			ul.unlock();
		}

		//virtual Timings * clone() const override {
		//	return new SpreadTimings(threads, min, max);
		//}

		virtual ostream & formatHeader(ostream & out) const override {
			return out << "SpreadHeader" << endl;
		}

		virtual ostream & formatCSV(ostream & out) const override {
			return out << "SpreadFields" << endl;
		}

		virtual ostream & formatHuman(ostream & out) const override {
			return out
				<< "threads: " << threads << " spread: " << max - min << endl;
		}

	private:
		SpreadTimings(unsigned t, long long unsigned i, long long unsigned a)
			: threads(t), min(i), max(a), mutexthis()
		{}

		unsigned threads;
		long long unsigned min;
		long long unsigned max;
		mutex mutexthis;
};

//
// SIMPLE
//

class TestSingle: public SingleBenchmark {
	public:
		virtual void runSingle() final override {
			long long unsigned start = rdtsc();
			cout << "I am Simple!" << endl;
			long long unsigned stop = rdtsc();
			cout << PhonyTimings(start, stop).asHuman() << endl;
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
			spread()
	{}

		virtual TestThreaded * clone() const override {
			return new TestThreaded(minThreads(), maxThreads(), iterations.getMax<0>());
		}

		virtual void init(unsigned) final override {
			const unsigned nthr = numThreads();
			shared = new unsigned[nthr];
			spread.reset();
		}

		virtual void ready(unsigned threadNum) final override {
			shared[threadNum] = threadNum;
		}

		virtual void set(unsigned threadNum) final override {
			shared[threadNum] += 1;
		}

		virtual void go(unsigned threadNum) final override {
			go_wait_start();
			const long long unsigned start = rdtsc();
			shared[threadNum] *= 2;
			// sync before executing non-benchmarked operations
			go_wait_end();
			spread.addStamp(start);
		}

		virtual void finish(unsigned) final override {
			delete[] shared;
			cout << spread.asHuman();
		}

		virtual void setUp() final override {
			ThreadedBenchmark::setUp();
			runThreads();
		}

	private:
		// TODO: do something with these iterations :)
		RangeSet<unsigned, unsigned> iterations;
		unsigned * shared;
		SpreadTimings spread;
};

int main(int argc, char * argv[]) {
	// default to 16 threads max
	unsigned nthreads = 16;

	switch (argc) {
		case 0: // huh? :-)
			break;
		case 1: // no arguments
			break;
		default: // too many arguments
			cerr << "NOTE: " << *argv
				<< " ignored all but first argument" << endl;
		case 2: // argument: expected number of threads
			nthreads = (unsigned)atoi(argv[1]);
			break;
	}

	{ // single run
		auto && ts = TestSingle();
		cout << "TestSingle range-based for:" << endl;
		for (auto & tmp: ts) { cout << tmp << endl; }
		cout << "TestSingle run:" << endl;
		ts.run();
	}

	{ // threaded
		auto && tt = TestThreaded(0, nthreads, 5);
		cout << "TestThreaded range-based for:" << endl;
		for (auto & tmp: tt) { cout << tmp << endl; }
		cout << endl << "TestThreaded run:" << endl;
		tt.run();
	}

	return 0;
}
