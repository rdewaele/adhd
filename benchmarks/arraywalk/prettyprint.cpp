#include "prettyprint.hpp"

using namespace std;

namespace prettyprint {

	ostream & operator<<(ostream & out, const Bytes & b) {
		static const struct {
			uint64_t bound;
			string suffix;
		} table[] = {
			{ (uint64_t) 1 << 60, "EiB" },
			{ (uint64_t) 1 << 50, "PiB" },
			{ (uint64_t) 1 << 40, "TiB" },
			{ (uint64_t) 1 << 30, "GiB" },
			{ (uint64_t) 1 << 20, "MiB" },
			{ (uint64_t) 1 << 10, "KiB" }
		};
		for (auto & row : table)
			if (b.bytes >= row.bound)
				return out << (double) b.bytes / (double) row.bound
					<< " " << row.suffix;

		return out << b.bytes << " B";
	}

}
