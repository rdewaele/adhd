#pragma once

#include "prettyprint.hpp"

#include <functional>
#include <iostream>
#include <string>

namespace adhd {

	class Benchmark;
	class Config;
	class Timings;

	typedef std::function<void (const Timings & timings)> timing_cb;

	class BenchmarkFactory {
		public:
			virtual Benchmark * makeBenchmark(const Config & cfg) const = 0;
			virtual ~BenchmarkFactory() = default;
	};

	class Benchmark {
		public:
			virtual void run(timing_cb tcb) = 0;
			virtual ~Benchmark() {};
	};

	class SimpleBenchmark: public Benchmark {
		public:
			virtual void run(timing_cb tcb) final override;
			virtual void runBare(timing_cb tcb) = 0;
			virtual ~SimpleBenchmark() {};
	};

	class ThreadedBenchmark: public Benchmark {
		public:
			virtual void run(timing_cb tcb) final override;
			virtual void runBare(timing_cb tcb) = 0;
			virtual ~ThreadedBenchmark() {};
		private:
			unsigned minThreads;
			unsigned maxThreads;
	};

	class Config {
		template <typename T>
		T get(std::string & s);
	};

	class Timings: public prettyprint::CSV, public prettyprint::Human {
		public:
			virtual std::ostream & formatHeader(std::ostream & out) const override = 0;
			virtual std::ostream & formatCSV(std::ostream & out) const override = 0;
			virtual std::ostream & formatHuman(std::ostream & out) const override = 0;
	};
}
