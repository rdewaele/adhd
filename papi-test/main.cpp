#include <algorithm>
#include <iostream>
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
	cout << "num counters: " << counters << endl << endl;

	constexpr const int all_events[] = { PAPI_TOT_CYC, PAPI_TOT_INS, PAPI_L2_TCM };
	constexpr const decltype(counters) total_events = sizeof(all_events) / sizeof(*all_events);
	constexpr const char * all_events_names[total_events] = { "cycles", "instr", "l2 miss" };

	auto events = vector<int>(min(total_events, counters));
	for (int i = 0; i < events.size(); ++i)
		events[i] = all_events[i];

	vector<long_long> values(events.size());
	int errcode;

	// init computation state
	constexpr const unsigned len = 1 << 12; // approx. size for proof of concept
	auto chase = vector<unsigned>(len);
	chase[len - 1] = 0;
	for (unsigned i = 0; i < len - 1; ++i)
		chase[i] = i + 1;

	// start counters
	if ((errcode = PAPI_start_counters(events.data(), events.size())) != PAPI_OK)
		handle_error(errcode);

	// perf stat
	for (int i = 1; i < 10; ++i) {
		// pointer chase
		unsigned idx = 0;
		for (unsigned loops = 4 * 1000 * 1000; loops > 0; --loops)
			idx = chase[idx];
		cout << "final index: " << idx << endl;
		// read counters
		if ((errcode = PAPI_read_counters(values.data(), values.size())) != PAPI_OK)
			handle_error(errcode);
		for (size_t i = 0; i < values.size(); ++i)
			cout << ">>> " << all_events_names[i] << "\t=\t" << values[i] << endl;
		cout << endl;
	}

	// stop counters
	if ((errcode = PAPI_stop_counters(values.data(), values.size())) != PAPI_OK)
		handle_error(errcode);
	cout << "stop counters:" << endl;
	for (size_t i = 0; i < values.size(); ++i)
		cout << ">>> " << all_events_names[i] << "\t=\t" << values[i] << endl;

	return 0;
}
