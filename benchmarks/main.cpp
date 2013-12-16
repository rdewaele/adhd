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

//
// CONFIG
//

class PhonyConfig: public Config {
	public:
		PhonyConfig(int iters): iterations(iters) {}

		virtual PhonyConfig * clone() const final override {
			return new PhonyConfig(iterations);
		}

		virtual Iterator begin() const final override {
			return Iterator(PhonyState(iterations));
		}

		virtual Iterator end() const final override {
			return Iterator(PhonyState(0));
		}

		class PhonyState: public State {
			public:
				PhonyState(int _iteration): iteration(_iteration) {}
				PhonyState(const PhonyState & ps): iteration(ps.iteration) {}

				virtual bool equals(const State & s) const final override {
					const PhonyState & ps = dynamic_cast<const PhonyState &>(s);
					return ps.iteration == iteration;
				}

				virtual void next() final override {
					if (iteration > 0)
						--iteration;
				}

				virtual PhonyState * clone() const final override {
					return new PhonyState(*this);
				}

			private:
				int iteration;
		};

	private:
		const int iterations;
};

//
// SIMPLE
//

class TestSimple: public SimpleBenchmark {
	public:
		TestSimple():
			SimpleBenchmark(Config())
	{}

		virtual void runBare(timing_cb tcb) override {
			long long unsigned start = rdtsc();
			cout << "I am Simple!" << endl;
			long long unsigned stop = rdtsc();
			tcb(PhonyTimings(start, stop));
		}
};

//
// THREADED
//

class TestThreaded: public ThreadedBenchmark {
	private:
		class TestContext;
		class TestContextFactory;

	public:
		TestThreaded(int iterations):
			ThreadedBenchmark(PhonyConfig(iterations), TestContextFactory())
	{}

		virtual void runPhase(Phase p, unsigned threadNum) override {
			const char * str;
			switch (p) {
				case Phase::INIT:
					str = "init";
					break;
				case Phase::READY:
					str = "ready";
					break;
				case Phase::SET:
					str = "set";
					break;
				case Phase::GO:
					str = "go";
					break;
				case Phase::FINISH:
					str = "finish";
					break;
			}
			cout << "thread " << threadNum << ": " << str << endl;
		}

	private:
		class TestContext: public ThreadedBenchmark::Context {
			public:
				TestContext(const unsigned nthr):
					Context(nthr)
			{}
		};

		class TestContextFactory: public ThreadedBenchmark::ContextFactory {
			public:
				virtual TestContext * makeContext(unsigned numThreads) const override {
					return new TestContext(numThreads);
				}
		};

#if 0
		virtual void * runThread(unsigned threadNum) override {
			int data = 42;
			while (syncSetup(threadNum)) { // do some shared var magic to return same val - also move outside this runThread ?
				syncWarmup(threadNum);
				for (int power = 0; power < 2; ++power) {
					syncStart(threadNum);
					long long unsigned start = rdtsc();
					int result;
					if (power) {
						result = data;
						for (int i = 0; i < power; ++i)
							data *= data;
					} else {
						result = 1;
					}
					cout << "result: " << result << endl;
					/*
						 long long unsigned stop = rdtsc();
						 auto temp = PhonyTimings(start, stop);
						 cout << temp.asHuman();
						 reportTimings(threadNum, temp);
						 */
				}
			}
			return NULL;
		}
#endif
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
		auto * threaded = new TestThreaded(3);
		threaded->run(phonyCallback);
		delete threaded;
	}
	return 0;
}
