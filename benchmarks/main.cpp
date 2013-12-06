#include "benchmark.hpp"

#include "rdtsc.h"

#include <iostream>

using namespace std;
using namespace adhd;

class PhonyTimings: public Timings {
	public:
		PhonyTimings():
			start(0),
			stop(0)
	{}

		PhonyTimings(long long unsigned _start, long long unsigned _stop):
			start(_start),
			stop(_stop)
	{}

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

class TestSimple: public SimpleBenchmark {
	virtual void runBare(timing_cb tcb) override {
		long long unsigned start = rdtsc();
		cout << "I am Simple!" << endl;
		long long unsigned stop = rdtsc();
		tcb(PhonyTimings(start, stop));
	}
};

class TestThreaded: public ThreadedBenchmark {
	public:
		TestThreaded():
			ThreadedBenchmark(Config())
	{}

		virtual void setup(unsigned numThreads) override {
			cout << "setup " << numThreads << " threads" << endl;
		}

		virtual void warmup() override {
			cout << "warmup" << endl;
		}

		virtual void runBare(unsigned threadNum) override {
			long long unsigned start = rdtsc();
			cout << "I am Threaded!" << endl;
			long long unsigned stop = rdtsc();
			auto temp = PhonyTimings(start, stop);
			cout << temp.asHuman();
			reportTimings(threadNum, temp);
		}
};

int main() {
	timing_cb phonyCallback = [] (const Timings & timings) {
		cout << "header: "; timings.formatHeader(cout);
		cout << "csv: " << timings.asCSV();
		cout << "human: " << timings.asHuman() << endl;
	};
	{
		auto simple = TestSimple();
		simple.run(phonyCallback);
	}
	{
		auto * threaded = new TestThreaded();
		threaded->run(phonyCallback);
		delete threaded;
	}
	return 0;
}
