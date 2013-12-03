#pragma once

#include "../benchmark.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>

namespace arraywalk {

	struct TimingData {
		uint64_t cycles;
		uint64_t reads;
		size_t length;
		size_t idx_size;
		unsigned istreams;
	};
	static_assert(std::is_pod<TimingData>::value, "struct TimingData must be a POD");

	class Timings: public adhd::Timings {
		public:
			Timings(const TimingData & td);
			virtual Timings * clone() const override;
			virtual std::ostream & formatHeader(std::ostream & out) const override;
			virtual std::ostream & formatCSV(std::ostream & out) const override;
			virtual std::ostream & formatHuman(std::ostream & out) const override;

		private:
			TimingData td;
	};

}
