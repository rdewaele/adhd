#include "prettyprint.hpp"

#include <iostream>

using namespace std;

namespace prettyprint {

	//
	// class Format
	//

	ostream & operator<<(ostream & out, const Format & f) {
		return f.format(out);
	}


	//
	// class Bytes
	//

	Bytes::Bytes(uint64_t _bytes):
		bytes(_bytes)
	{}

	ostream & Bytes::formatBytes(ostream & out) const {
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
			if (bytes >= row.bound)
				return out << (double) bytes / (double) row.bound
					<< " " << row.suffix;

		return out << bytes << " B";
	}

}
