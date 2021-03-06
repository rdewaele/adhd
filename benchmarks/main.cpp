#include "benchmark.hpp"
#include "hwcounters.hpp"

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
		virtual void run(timing_cb) final override {
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

class TestThreaded: public ThreadedBenchmark, public RangeSet<AffineStepper<unsigned>, AffineStepper<unsigned>> {
	public:
		TestThreaded(unsigned min, unsigned max, unsigned iters)
			: ThreadedBenchmark(min, max),
			RangeSet(AffineStepper<unsigned>(0, iters), AffineStepper<unsigned>(1, 2)),
			shared(nullptr),
			spread()
	{}

		virtual TestThreaded * clone() const override {
			return new TestThreaded(minThreads(), maxThreads(), getMaxValue<0>());
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

#if 0
		// regular loop nesting
		virtual void next() final override {
			RangeSet::next();
			if (RangeSet::atMin())
				ThreadedBenchmark::next();
		}
#else
		// loop inversion as proof of concept
		// (slower because more threads are being created in total)
		virtual void next() final override {
			ThreadedBenchmark::next();
			if (ThreadedBenchmark::atMin())
				RangeSet::next();
		}
#endif

		bool operator==(const TestThreaded & rhs) const {
			return static_cast<const RangeSet &>(*this) == rhs
				&& static_cast<const ThreadedBenchmark &>(*this) == rhs;
		}

		bool operator!=(const TestThreaded & rhs) const {
			return static_cast<const RangeSet &>(*this) != rhs
				|| static_cast<const ThreadedBenchmark &>(*this) != rhs;
		}

		virtual bool atMax() const final override {
			return ThreadedBenchmark::atMax() && RangeSet::atMax();
		}

		virtual bool atMin() const final override {
			return ThreadedBenchmark::atMin() && RangeSet::atMin();
		}

		virtual void gotoBegin() final override {
			ThreadedBenchmark::gotoBegin();
			RangeSet::gotoBegin();
		}

		virtual void gotoEnd() final override {
			ThreadedBenchmark::gotoEnd();
			RangeSet::gotoEnd();
		}

		friend inline ostream & operator<<(ostream & os, const TestThreaded & tt) {
			return os << static_cast<const ThreadedBenchmark &>(tt)
				<< " | TestThreaded: " << static_cast<const RangeSet &>(tt);
		}

	private:
		unsigned * shared;
		SpreadTimings spread;
};

enum class Count { ONE=111, TWO=1, THREE=11 };
ostream & operator<<(ostream & os, const Count & c) {
	const char * str;
	switch (c) {
		case Count::ONE: str = "ONE"; break;
		case Count::TWO: str = "TWO"; break;
		case Count::THREE: str = "THREE"; break;
		default: str = "<unknown>"; break;
	}
	return os << str;
}

int main(int argc, char * argv[]) {
	// hwcounters - start cache profile of main
	const Events events {
		hwcounters::cache::L1::DCA,
			hwcounters::cache::L1::DCH,
			hwcounters::cache::L1::DCM,
	};
	auto ctrs = PerfStat(events);
	ctrs.start();

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

	{ // count down using an explicit stepper
		auto stepper = ExplicitStepper<Count>{ Count::THREE, Count::TWO, Count::ONE };
		for (auto && es: stepper)
			cout << es << endl;
	}

	timing_cb tcb = [] (const Timings &) {};

	{ // single run
		auto && ts = TestSingle();
		cout << "TestSingle range-based for:" << endl;
		for (auto & tmp: ts) { cout << tmp << endl; }
		cout << "TestSingle run:" << endl;
		runBenchmark(ts, tcb);
	}

	{ // threaded
		auto && tt = TestThreaded(1, nthreads, 1);
		cout << "TestThreaded range-based for:" << endl;
		for (auto & tmp: tt) { cout << tmp << endl; }
		cout << endl << "TestThreaded run:" << endl;
		runBenchmark(tt, tcb);
	}

	// hwcounters - end
	cout << ctrs.stop();
	return 0;
}
