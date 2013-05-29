#include "logging.h"

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

const char * bool2yesno(bool b) { return b ? "yes" : "no"; }
const char * bool2onoff(bool b) { return b ? "on" : "off"; }

void CSV_LogTimings(
		FILE * log,
		long long id,
		struct walkArray * wa,
		nsec_t nsec,
		nsec_t stddev
		)
{
	if (log)
		fprintf(log,
				"%lld,%zu,%llu,%llu\n",
				id, wa->size, nsec, stddev);
}

// log to stdout depending on verbosity
void verbose(const struct options * options, const char *format, ...) {
	va_list args;
	va_start(args, format);

	if(!options->generic.silent)
		vprintf(format, args);

	va_end(args);
}

// walkArray creation report
void logMakeWalkArray(
		const struct options * const options,
		const struct walkArray * const array,
		struct timespec * elapsed)
{
	const char * array_unit;
	walking_t array_size;
	const uint32_t kilo = 1 << 10;
	const uint32_t mega = 1 << 20;
	const uint32_t giga = 1 << 30;
	if (array->size < kilo) {
		array_unit = "B";
		array_size = array->size;
	}
	else if (array->size < mega) {
		array_unit = "KiB";
		array_size = array->size / kilo;
	}
	else if (array->size < giga) {
		array_unit = "MiB";
		array_size = array->size / mega;
	}
	else {
		array_unit = "GiB";
		array_size = array->size / giga;
	}
	nsec_t totalusec = timespecToNsec(elapsed) / 1000;
	verbose(options,
			"%.4lu %s (= %lu elements) randomized in %"PRINSEC" usec | %u reads:\n",
			array_size, array_unit, array->len, totalusec,
			options->walkArray.aaccesses);
}

void logWalkArray(
		const struct options * const options,
		const nsec_t * const timings,
		nsec_t old_avg)
{
	const struct options_walkarray * const wa_opt = &(options->walkArray);
	const struct options_generic * const gn_opt = &(options->generic);
	assert(wa_opt->repetitions > 0);
	// average
	nsec_t totalnsec = 0;
	for (size_t i = 0; i < wa_opt->repetitions; ++i)
		totalnsec += timings[i];
	// XXX whole division should be OK: timings are in the millions of nsec
	nsec_t new_avg = totalnsec / wa_opt->repetitions;
	if (0 == old_avg)
		old_avg = new_avg;

	// standard deviation
	totalnsec = 0;
	for (size_t i = 0; i < wa_opt->repetitions; ++i) {
		nsec_t current = timings[i];
		current = current > new_avg ? current - new_avg : new_avg - current;
		totalnsec += current * current;
	}
	double stddev = sqrt((double)(totalnsec / wa_opt->repetitions));

	// report results
	// TODO
	//CSV_LogTimings(csvlog, pid, array, new_avg, lround(stddev));

	// timing for a single run
	verbose(options, ">>>\t%"PRINSEC" usec"
			" | delta %+2.2lf%%"
			" (%"PRINSEC" -> %"PRINSEC")"
			" | stddev %ld usec (%2.2lf%%)\n",
			new_avg / 1000,
			100 * (double)(new_avg - old_avg) / (double)old_avg,
			old_avg, new_avg,
			lround(stddev / 1000), 100 * stddev / (double)new_avg
			);

	// timing for a single read
	nsec_t nsread_old = old_avg / wa_opt->aaccesses;
	nsec_t nsread_new = new_avg / wa_opt->aaccesses;
	verbose(options, ">>>\t~%"PRINSEC" nsec/read"
			" | delta %+2.2lf%%"
			" (%"PRINSEC" -> %"PRINSEC")"
			" | ~%.2lf cycles/read"
			" @ %.3f GHz"
			"\n",
			nsread_new,
			100 * (double)(nsread_new - nsread_old) / (double)nsread_old,
			nsread_old, nsread_new,
			((double)gn_opt->frequency * (double)new_avg) / (double)wa_opt->aaccesses,
			gn_opt->frequency);

	// bandwidth estimation for a single run
	double totalbytes = (double)wa_opt->aaccesses * sizeof(walking_t);
	double tb_new = totalbytes / (double)new_avg;
	double tb_old = totalbytes / (double)old_avg;
	verbose(options, ">>>\t~%.3lf GiB/s"
			" | delta %+2.2lf%%"
			" (%.3lf -> %.3lf)\n",
			tb_new,
			100 * (tb_new - tb_old) / tb_old,
			tb_old, tb_new);

	verbose(options, "\n");
}
