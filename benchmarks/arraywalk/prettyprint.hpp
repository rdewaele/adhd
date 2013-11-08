#pragma once

#include <cstdint>
#include <iostream>

namespace prettyprint {
	struct Bytes {
		uint64_t bytes;
		friend std::ostream & operator<<(std::ostream & out, const Bytes & b);
	};
}
