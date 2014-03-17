#include <algorithm>
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
	cout << "num counters: " << counters << endl << endl;

	constexpr const int all_events[] = { PAPI_TOT_INS, PAPI_TOT_CYC, PAPI_L1_DCM, PAPI_L1_ICM };
	constexpr const decltype(counters) total_events = sizeof(all_events) / sizeof(*all_events);
	constexpr const char * all_events_names[total_events] = { "instr", "cycles", "l1 D$ miss", "l1 I$ miss" };

	auto events = vector<int>(min(total_events, counters));
	for (int i = 0; i < events.size(); ++i)
		events[i] = all_events[i];

	vector<long_long> values(events.size());
	int errcode;

	// init computation state
	// guesstimate L1 D$ size to be no smaller than 4KiB
	constexpr const unsigned len = 1 << 10;
	constexpr const unsigned num_loops = 4 * 1000 * 1000;
	cout << "Testing linear read with " << sizeof(unsigned) * len / 1024 << "KiB and "
		<< num_loops << " reads." << endl << endl;

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
		for (unsigned loops = num_loops; loops > 0; --loops)
			idx = chase[idx];
		cout << "final index: " << idx << endl;
		// read counters
		if ((errcode = PAPI_read_counters(values.data(), values.size())) != PAPI_OK)
			handle_error(errcode);
		for (size_t i = 0; i < values.size(); ++i)
			cout << ">>> " << all_events_names[i] << "\t=\t" << values[i]
				<< "\t(/reads: " << static_cast<double>(values[i]) / num_loops << ")" << endl;
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
