#include "benchmark.hpp"

#include <iostream>

using namespace std;
using namespace adhd;

class PhonyTimings: public Timings {
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
		return out << "PhonyHuman" << endl;
	}
};

class TestSimple: public SimpleBenchmark {
	virtual void runBare(timing_cb tcb) override {
		cout << "I am Simple!" << endl;
		tcb(PhonyTimings());
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
			cout << "I am Threaded!" << endl;
			reportTimings(threadNum, PhonyTimings());
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
		auto threaded = TestThreaded();
		threaded.run(phonyCallback);
	}
	return 0;
}
