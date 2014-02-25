#include <iostream>
#include <stdexcept>
#include <vector>

#include <papi.h>

using namespace std;

inline void handle_error(int retval) {
	throw runtime_error(PAPI_strerror(retval));
}

int main() {
	int counters;
	if ((counters = PAPI_num_counters()) <= PAPI_OK)
		handle_error(counters);
	cout << "num counters: " << counters << endl;

	if (counters < 3)
		throw runtime_error("not enough counters available: need at least 3");

	vector<int> events = { PAPI_TOT_CYC, PAPI_TOT_INS, PAPI_L2_TCM };
	cout << "counter names:\t cycles\t instr\t l2 miss" << endl;
	vector<long_long> values(events.size());
	int errcode;

	// start counters
	if ((errcode = PAPI_start_counters(events.data(), events.size())) != PAPI_OK)
		handle_error(errcode);

	// do computations
	// ...

	// read counters
	for (int i = 1; i < 5; ++i) {
		if ((errcode = PAPI_read_counters(values.data(), values.size())) != PAPI_OK)
			handle_error(errcode);
		cout << "read counters:\t";
		for (const auto & tmp: values)
			cout << "| " << tmp << "\t";
		cout << "|" << endl;
	}

	// stop counters
	if ((errcode = PAPI_stop_counters(values.data(), values.size())) != PAPI_OK)
		handle_error(errcode);
	cout << "stop counters:\t";
	for (const auto & tmp: values)
		cout << "| " << tmp << "\t";
	cout << "|" << endl;

	return 0;
}
