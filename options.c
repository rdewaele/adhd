#include "options.h"

#include <libconfig.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

// these are meta options; the are not present in the option structure
// ? h - explicit help request (no invalid option warning)
// (c)onfiguration file to use
// (g)enerate default configuration file
// (i)nformation about run configuration
static const char * OPTSTR = "h?c:gi";

// 'begin' default is just 0, but defined here to be consistent
#define BEGIN_INIT 0

// 'end' default must (only) be lowered when it doesn't fit in walking_t
#if WALKING_MAX >= 1 << 23
#define END_INIT 1 << 23
#elif WALKING_MAX >= ((1 << 16) - 1)
#define END_INIT ((1 << 16) - 1)
#else // < 2^16
#define END_INIT ((1 << 8) - 1)
#endif

// 'step' default must (only) be lowered when it doesn't fit in walking_t
#if WALKING_MAX >= 1 << 12
#define STEP_INIT 1 << 12
#else // < 2^12
#define STEP_INIT 1 << 4
#endif

static const char FLD_ARRAY_GROUP[] = "array";
static const char FLD_ARRAY_ACCESSES[] = "accesses";
static const char FLD_ARRAY_REPEAT[] = "repeat";
static const char FLD_ARRAY_BEGINLENGTH[] = "beginlength";
static const char FLD_ARRAY_ENDLENGTH[] = "endlength";
static const char FLD_ARRAY_INCREMENT[] = "increment"; 
static const char FLD_ARRAY_SCALING[] = "scaling";
static const char FLD_ARRAY_PATTERN[] = "pattern";
static const char FLD_CPU_FREQUENCY[] = "CPU_frequency";
static const char FLD_LOGGING[] = "logging";
static const char FLD_LOGFILE[] = "logfile";
static const char FLD_SPAWN[] = "spawn";
static const char FLD_PROCESSES[] = "processes";
static const char FLD_SILENT[] = "silent";
static const char FLD_THREADS[] = "threads";

// load the default configuration in cfg
static void set_default_config(config_t * cfg) {
	static const unsigned AACCESSES = 4 * 1024 * 1024;
	static const unsigned BEGIN = BEGIN_INIT;
	static const char SPAWN[] = "tree";
	static const long long END = END_INIT;
	static const float FREQUENCY = 1;
	static const bool LOGGING = false;
	static const char LOGFILE[] = "adhd_log";
	static const char PATTERN[] = "random";
	static const unsigned PROCESSES = 1;
	static const unsigned REPETITIONS = 50;
	static const long long STEP = STEP_INIT;
	static const char SCALING[] = "linear";
	static const bool SILENT = false;
	static const unsigned THREADS = 1;

	config_setting_t *setting;
	config_setting_t *root;
	root = config_root_setting(cfg);

	// array group
	{
		config_setting_t *array =
			config_setting_add(root, FLD_ARRAY_GROUP, CONFIG_TYPE_GROUP);

		// array accesses
		setting = config_setting_add(array, FLD_ARRAY_ACCESSES, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, AACCESSES);

		// repetitions of a single test, to be able to calculate an average
		// and a standard deviation, indicating run precision
		setting = config_setting_add(array, FLD_ARRAY_REPEAT, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, REPETITIONS);

		// initial array size
		setting = config_setting_add(array, FLD_ARRAY_BEGINLENGTH, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, BEGIN);

		// maximal array size
		setting = config_setting_add(array, FLD_ARRAY_ENDLENGTH, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, END);

		// increment factor
		setting = config_setting_add(array, FLD_ARRAY_INCREMENT, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, STEP);

		// increment type
		// (linear adds the increment, exponential multiplies the previous length)
		setting = config_setting_add(array, FLD_ARRAY_SCALING, CONFIG_TYPE_STRING);
		config_setting_set_string(setting, SCALING);

		// walking pattern
		// (sequentially increasing/decreasing or random)
		setting = config_setting_add(array, FLD_ARRAY_PATTERN, CONFIG_TYPE_STRING);
		config_setting_set_string(setting, PATTERN);
	}

	// miscellaneous
	{
		// CPU frequency to calculate cycle estimate
		// TODO: use actual cycle counter in the CPU
		// (this is a feature with a design impact, as SMP complicates matters ... :))
		setting = config_setting_add(root, FLD_CPU_FREQUENCY, CONFIG_TYPE_FLOAT);
		config_setting_set_float(setting, FREQUENCY);

		// logging enabled?
		setting = config_setting_add(root, FLD_LOGGING, CONFIG_TYPE_BOOL);
		config_setting_set_bool(setting, LOGGING);

		// base filename to log process timings to
		setting = config_setting_add(root, FLD_LOGFILE, CONFIG_TYPE_STRING);
		config_setting_set_string(setting, LOGFILE);

		// process spawn method
		setting = config_setting_add(root, FLD_SPAWN, CONFIG_TYPE_STRING);
		config_setting_set_string(setting, SPAWN);

		// number of processes to run
		// TODO: just like arrays we could have min/max and a linear or exponential increment
		setting = config_setting_add(root, FLD_PROCESSES, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, PROCESSES);

		// number of threads per process to run
		// TODO: just like arrays we could have min/max and a linear or exponential increment
		setting = config_setting_add(root, FLD_THREADS, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, THREADS);

		// silent mode
		setting = config_setting_add(root, FLD_SILENT, CONFIG_TYPE_BOOL);
		config_setting_set_bool(setting, SILENT);
	}
}

static void c2o_strncpy(
		char * const dst,
		const config_setting_t * const src,
		const size_t len)
{
	const char * tmp = config_setting_get_string(src);
	strncpy(dst, tmp, len);
	dst[len - 1] = '\0';
}

// TODO: stop abusing NAME_MAX ;-)
// TODO: proper bounds checking for all types
static void config2options(const config_t * config, struct options * options) {
	// array
	long long aaccesses;
	long long begin;
	long long repetitions;
	long long end;
	long long step;
	char scaling[NAME_MAX];
	char pattern[NAME_MAX];
	// misc
	double frequency;
	int logging;
	char logfile[NAME_MAX];
	char create[NAME_MAX];
	long long processes;
	long long threads;
	int silent;

	// array group
	{
		config_setting_t * array = config_lookup(config, FLD_ARRAY_GROUP);
		config_setting_lookup_int64(array, FLD_ARRAY_ACCESSES, &aaccesses);
		config_setting_lookup_int64(array, FLD_ARRAY_REPEAT, &repetitions);
		config_setting_lookup_int64(array, FLD_ARRAY_BEGINLENGTH, &begin);
		config_setting_lookup_int64(array, FLD_ARRAY_ENDLENGTH, &end);
		config_setting_lookup_int64(array, FLD_ARRAY_INCREMENT, &step);
		c2o_strncpy(scaling,
				config_setting_get_member(array, FLD_ARRAY_SCALING), NAME_MAX);
		c2o_strncpy(pattern,
				config_setting_get_member(array, FLD_ARRAY_PATTERN), NAME_MAX);
	}
	// miscellaneous
	{
		config_lookup_float(config, FLD_CPU_FREQUENCY, &frequency);
		config_lookup_bool(config, FLD_LOGGING, &logging);
		c2o_strncpy(logfile, config_lookup(config, FLD_LOGFILE), NAME_MAX);
		c2o_strncpy(create, config_lookup(config, FLD_SPAWN), NAME_MAX);
		config_lookup_int64(config, FLD_PROCESSES, &processes);
		config_lookup_int64(config, FLD_THREADS, &threads);
		config_lookup_bool(config, FLD_SILENT, &silent);
	}
	struct options tmp = {
		(unsigned)aaccesses,
		(walking_t)begin,
		spawn_typeFromString(create),
		(walking_t)end,
		frequency,
		logging,
		"", // XXX strncpy ! (log filename),
		pattern_typeFromString(pattern),
		(unsigned)processes,
		(unsigned)repetitions,
		(walking_t)step,
		silent,
		(unsigned)threads
	};
	strncpy(tmp.csvlogname, logfile, NAME_MAX);
	memcpy(options, &tmp, sizeof(struct options));
}

static void options_help(const char * name) {
	fprintf(stderr, "Usage: %s [options]\noptions:\n"
			"\t-h -?     print this help message\n"
			"\t-c <file> read configuration from <file>\n"
			"\t-g        print default configuration file to stdout and quit\n"
			"\t-i        print run configuration at program startup\n",
			name);
}

static void options_print(const struct options * options) {
	fprintf(stderr,
			"array accesses to perform: %u\n"
			"process spawn method: %s\n"
			"initial array size: %"PRIWALKING"\n"
			"maximum array size: %"PRIWALKING"\n"
			"cpu clock frequency: %.3f\n"
			"CSV log base name: %s\n"
			"CSV logging enabled: %s\n"
			"array walking pattern: %s\n"
			"operating processes: %u\n"
			"threads per process: %u\n"
			"single test configuration repeat: %u\n"
			"array increment: %"PRIWALKING"\n"
			"silent mode: %s\n"
			"\n",
			options->aaccesses,
			spawn_typeToString(options->create),
			options->begin,
			options->end,
			options->frequency,
			options->csvlogname,
			options->logging ? "yes" : "no",
			pattern_typeToString(options->pattern),
			options->processes,
			options->threads,
			options->repetitions,
			options->step,
			options->Silent ? "on" : "off"
			);
}

// TODO: error handling
// TODO: better exit point handling
// (freeing resources for valgrind-cleanliness is too tedious at the moment)
void options_parse(int argc, char * argv[], struct options * options) {
	int opt;
	// modified when config is loaded on the command line
	config_t config;
	config_init(&config);
	set_default_config(&config);

	// only to used for -g: print default config
	config_t config_default;
	config_init(&config_default);
	set_default_config(&config_default);

	// -i flag should only print options after all other
	// command line arguments have been applied
	bool print_options = false;

	while (-1 != (opt = getopt(argc, argv, OPTSTR))) {
		switch (opt) {
			case 'c':
				config_set_auto_convert(&config, CONFIG_TRUE);

				if (CONFIG_FALSE == config_read_file(&config, optarg)) {
					// failure reporting
					fprintf(stderr, "Failed to use config file '%s'!\n", optarg);
					switch (config_error_type(&config)) {
						case CONFIG_ERR_NONE:
							fprintf(stderr, "\tno error reported\n"
									"\t(This might be a libconfig problem.)\n");
							break;
						case CONFIG_ERR_FILE_IO:
							fprintf(stderr, "\tfile I/O error\n");
							break;
						case CONFIG_ERR_PARSE:
							fprintf(stderr, "\tparse error on line %d:\n"
									"\t%s\n",
									config_error_line(&config),
									config_error_text(&config));
							break;
						default:
							fprintf(stderr, "\tunknown error\n"
									"\t(A new libconfig version might have introduced"
									" new kinds of warnings.)\n");
					}
				}
				break;
			case 'g':
				config_write(&config_default, stdout);
				// be valgrind-clean
				config_destroy(&config_default);
				config_destroy(&config);
				exit(EXIT_SUCCESS);
				break;
			case 'i':
				print_options = true;
				break;
			default: /* '?' 'h' */
				options_help(*argv);
				// be valgrind-clean
				config_destroy(&config_default);
				config_destroy(&config);
				exit(EXIT_FAILURE);
		}
	}

	config2options(&config, options);
	// be valgrind-clean
	config_destroy(&config_default);
	config_destroy(&config);

	if (print_options)
		options_print(options);
}

#define CASE_ENUM2STRING(ENUM) case ENUM: return #ENUM
const char * spawn_typeToString(enum spawn_type st) {
	switch (st) {
		CASE_ENUM2STRING(TREE);
		CASE_ENUM2STRING(LINEAR);
		default:
		return NULL;
	}
}

const char * pattern_typeToString(enum pattern_type pt) {
	switch (pt) {
		CASE_ENUM2STRING(RANDOM);
		CASE_ENUM2STRING(INCREASING);
		CASE_ENUM2STRING(DECREASING);
		default:
		return NULL;
	}
}

#define STRING2ENUM(SRC,ENUM) if (0 == strcasecmp(#ENUM, SRC)) return ENUM
enum spawn_type spawn_typeFromString(const char * st) {
	STRING2ENUM(st, TREE);
	STRING2ENUM(st, LINEAR);
	return TREE;
}

enum pattern_type pattern_typeFromString(const char * pt) {
	STRING2ENUM(pt, RANDOM);
	STRING2ENUM(pt, INCREASING);
	STRING2ENUM(pt, DECREASING);
	return RANDOM;
}
