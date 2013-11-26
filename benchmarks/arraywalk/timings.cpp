#include "timings.hpp"

#include "../prettyprint.hpp"

#include <iostream>

using namespace prettyprint;
using namespace std;

/* icpc warns that 'args' in sequence is unreferenced, which is untrue
 * we assume the compiler gets confused by the variadic templates
 * furthermore, we cannot enable the warning again for this file because icpc
 * warns when expanding the template, which apparently happens after reading
 * this complete source
 * (last checked with icpc (ICC) 14.0.1 20131008) */
#ifdef __INTEL_COMPILER
#pragma warning(disable:869)
#endif

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
		out << td.length << " elements x " << Bytes(td.idx_size) << " = "
			<< Bytes(td.length * td.idx_size) << " | " << td.istreams
			<< " instruction streams" << endl;
		out << "cycles: " << td.cycles << " | ";
		out << "reads: " << td.reads << " (" << Bytes(td.reads * td.idx_size) << ")" << endl;
		out << "~cycles per read: "
			<< (double) td.cycles / (double) td.reads << endl;
		return out;
	}

}
