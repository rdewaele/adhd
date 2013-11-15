#pragma once

#include <type_traits>

namespace arraywalk {
	struct Timings {
		uint64_t cycles;
		uint64_t reads;
		size_t length;
		unsigned istreams;
	};
	static_assert(std::is_pod<Timings>::value, "struct Timings must be a POD");
}
