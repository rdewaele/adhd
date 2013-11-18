#include "prettyprint.hpp"
#include "timings.hpp"

#include <iostream>

using namespace prettyprint;
using namespace std;

namespace arraywalk {

	Timings::Timings(const TimingData & _td):
		td(_td)
	{}

	ostream & Timings::formatHeader(ostream & out) const {
		out << "cycles, reads, elements, element size, instruction streams" << endl;
		return out;
	}

	ostream & Timings::formatCSV(ostream & out) const {
		return sequence(out, td.cycles, td.reads, td.length, td.idx_size, td.istreams);
	}

	ostream & Timings::formatHuman(ostream & out) const {
		out << td.length << " elements x " << Bytes{td.idx_size} << " = "
			<< Bytes{td.length * td.idx_size} << " | " << td.istreams
			<< " instruction streams" << endl;
		out << "cycles: " << td.cycles << " | ";
		out << "reads: " << td.reads << " (" << Bytes{td.reads * td.idx_size} << ")" << endl;
		out << "~cycles per read: "
			<< (double) td.cycles / (double) td.reads << endl;
		return out;
	}

}
