#include "prettyprint.hpp"

using namespace std;

namespace prettyprint {
	ostream & operator<<(ostream & out, const Bytes & b) {
		struct {
			uint64_t bound;
			string suffix;
		} table[] = {
			{ (uint64_t) 1 << 10, "B" },
			{ (uint64_t) 1 << 20, "KiB" },
			{ (uint64_t) 1 << 30, "MiB" },
			{ (uint64_t) 1 << 40, "GiB" },
			{ (uint64_t) 1 << 50, "TiB" },
			{ (uint64_t) 1 << 60, "PiB" }
		};
		for (auto & row : table)
			if (b.bytes < row.bound)
				return out << (double) b.bytes / (double) (row.bound >> 10)
					<< " " << row.suffix;

		// unreachable - suppress "control reaches end of non-void function"
		return out;
	}
}
