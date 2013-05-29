#include "options.h"
#include "logging.h"

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

static const char FLD_GENERIC_GROUP[] = "generic";
static const char FLD_GENERIC_GPU_FREQUENCY[] = "CPU_frequency";
static const char FLD_GENERIC_LOGGING[] = "logging";
static const char FLD_GENERIC_LOGFILE[] = "logfile";
static const char FLD_GENERIC_SPAWN[] = "spawn";
static const char FLD_GENERIC_PROCESSES[] = "processes";
static const char FLD_GENERIC_SILENT[] = "silent";
static const char FLD_GENERIC_THREADS[] = "threads";

static const char FLD_WALKARRAY_GROUP[] = "walkarray";
static const char FLD_WALKARRAY_ACCESSES[] = "accesses";
static const char FLD_WALKARRAY_REPEAT[] = "repeat";
static const char FLD_WALKARRAY_BEGINLENGTH[] = "beginlength";
static const char FLD_WALKARRAY_ENDLENGTH[] = "endlength";
static const char FLD_WALKARRAY_INCREMENT[] = "increment";
static const char FLD_WALKARRAY_SCALING[] = "scaling";
static const char FLD_WALKARRAY_PATTERN[] = "pattern";

static const char FLD_STREAMARRAY_GROUP[] = "streamarray";
static const char FLD_STREAMARRAY_BEGINLENGTH[] = "beginlength";
static const char FLD_STREAMARRAY_ENDLENGTH[] = "endlength";
static const char FLD_STREAMARRAY_INCREMENT[] = "increment";
static const char FLD_STREAMARRAY_SCALING[] = "scaling";

static const char FLD_FLOPSARRAY_GROUP[] = "flopsarray";
static const char FLD_FLOPSARRAY_LENGTH[] = "length";

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

	// generic group
	{
		config_setting_t *generic =
			config_setting_add(root, FLD_GENERIC_GROUP, CONFIG_TYPE_GROUP);

		// CPU frequency to calculate cycle estimate
		// TODO: use actual cycle counter in the CPU
		// (this is a feature with a design impact, as SMP complicates matters ... :))
		setting = config_setting_add(generic, FLD_GENERIC_GPU_FREQUENCY, CONFIG_TYPE_FLOAT);
		config_setting_set_float(setting, FREQUENCY);

		// logging enabled?
		setting = config_setting_add(generic, FLD_GENERIC_LOGGING, CONFIG_TYPE_BOOL);
		config_setting_set_bool(setting, LOGGING);

		// base filename to log process timings to
		setting = config_setting_add(generic, FLD_GENERIC_LOGFILE, CONFIG_TYPE_STRING);
		config_setting_set_string(setting, LOGFILE);

		// process spawn method
		setting = config_setting_add(generic, FLD_GENERIC_SPAWN, CONFIG_TYPE_STRING);
		config_setting_set_string(setting, SPAWN);

		// number of processes to run
		// TODO: just like arrays we could have min/max and a linear or exponential increment
		setting = config_setting_add(generic, FLD_GENERIC_PROCESSES, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, PROCESSES);

		// number of threads per process to run
		// TODO: just like arrays we could have min/max and a linear or exponential increment
		setting = config_setting_add(generic, FLD_GENERIC_THREADS, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, THREADS);

		// silent mode
		setting = config_setting_add(generic, FLD_GENERIC_SILENT, CONFIG_TYPE_BOOL);
		config_setting_set_bool(setting, SILENT);
	}

	// walking array group
	{
		config_setting_t *array =
			config_setting_add(root, FLD_WALKARRAY_GROUP, CONFIG_TYPE_GROUP);

		// array accesses
		setting = config_setting_add(array, FLD_WALKARRAY_ACCESSES, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, AACCESSES);

		// repetitions of a single test, to be able to calculate an average
		// and a standard deviation, indicating run precision
		setting = config_setting_add(array, FLD_WALKARRAY_REPEAT, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, REPETITIONS);

		// initial array size
		setting = config_setting_add(array, FLD_WALKARRAY_BEGINLENGTH, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, BEGIN);

		// maximal array size
		setting = config_setting_add(array, FLD_WALKARRAY_ENDLENGTH, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, END);

		// increment factor
		setting = config_setting_add(array, FLD_WALKARRAY_INCREMENT, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, STEP);

		// increment type
		// (linear adds the increment, exponential multiplies the previous length)
		setting = config_setting_add(array, FLD_WALKARRAY_SCALING, CONFIG_TYPE_STRING);
		config_setting_set_string(setting, SCALING);

		// walking pattern
		// (sequentially increasing/decreasing or random)
		setting = config_setting_add(array, FLD_WALKARRAY_PATTERN, CONFIG_TYPE_STRING);
		config_setting_set_string(setting, PATTERN);
	}

	// streaming array group
	{
		config_setting_t *array =
			config_setting_add(root, FLD_STREAMARRAY_GROUP, CONFIG_TYPE_GROUP);

		// initial array size
		setting = config_setting_add(array, FLD_STREAMARRAY_BEGINLENGTH, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, BEGIN);

		// maximal array size
		setting = config_setting_add(array, FLD_STREAMARRAY_ENDLENGTH, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, END);

		// increment factor
		setting = config_setting_add(array, FLD_STREAMARRAY_INCREMENT, CONFIG_TYPE_INT64);
		config_setting_set_int64(setting, STEP);

		// increment type
		// (linear adds the increment, exponential multiplies the previous length)
		setting = config_setting_add(array, FLD_STREAMARRAY_SCALING, CONFIG_TYPE_STRING);
		config_setting_set_string(setting, SCALING);
	}

	// flops array group
	{
		config_setting_t *array =
			config_setting_add(root, FLD_FLOPSARRAY_GROUP, CONFIG_TYPE_GROUP);

		// array size
		setting = config_setting_add(array, FLD_FLOPSARRAY_LENGTH, CONFIG_TYPE_INT);
		config_setting_set_int64(setting, END);
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

	// generic group
	struct options_generic gn_opt;
	{
		double frequency;
		int logging, silent;
		char logfile[NAME_MAX], create[NAME_MAX];
		long long processes, threads;

		config_setting_t * generic = config_lookup(config, FLD_GENERIC_GROUP);
		config_setting_lookup_float(generic, FLD_GENERIC_GPU_FREQUENCY, &frequency);
		config_setting_lookup_bool(generic, FLD_GENERIC_LOGGING, &logging);
		c2o_strncpy(logfile,
				config_setting_get_member(generic, FLD_GENERIC_LOGFILE), NAME_MAX);
		c2o_strncpy(create,
				config_setting_get_member(generic, FLD_GENERIC_SPAWN), NAME_MAX);
		config_setting_lookup_int64(generic, FLD_GENERIC_PROCESSES, &processes);
		config_setting_lookup_int64(generic, FLD_GENERIC_THREADS, &threads);
		config_setting_lookup_bool(generic, FLD_GENERIC_SILENT, &silent);

		gn_opt = (struct options_generic) {
			spawn_typeFromString(create),
				frequency,
				logging,
				"", // XXX strncpy ! (log filename)
				(unsigned)processes,
				silent,
				(unsigned)threads
		};
		strncpy(gn_opt.csvlogname, logfile, NAME_MAX);
	}

	// walking array group
	struct options_walkarray wa_opt;
	{
		long long aaccesses, begin, repetitions, end, step;
		char scaling[NAME_MAX], pattern[NAME_MAX];

		config_setting_t * array = config_lookup(config, FLD_WALKARRAY_GROUP);
		config_setting_lookup_int64(array, FLD_WALKARRAY_ACCESSES, &aaccesses);
		config_setting_lookup_int64(array, FLD_WALKARRAY_REPEAT, &repetitions);
		config_setting_lookup_int64(array, FLD_WALKARRAY_BEGINLENGTH, &begin);
		config_setting_lookup_int64(array, FLD_WALKARRAY_ENDLENGTH, &end);
		config_setting_lookup_int64(array, FLD_WALKARRAY_INCREMENT, &step);
		c2o_strncpy(scaling,
				config_setting_get_member(array, FLD_WALKARRAY_SCALING), NAME_MAX);
		c2o_strncpy(pattern,
				config_setting_get_member(array, FLD_WALKARRAY_PATTERN), NAME_MAX);

		wa_opt = (struct options_walkarray) {
			(unsigned)aaccesses,
				(walking_t)begin,
				(walking_t)end,
				pattern_typeFromString(pattern),
				(unsigned)repetitions,
				(walking_t)step
		};
	}

	// streaming array group
	struct options_streamarray sa_opt;
	{
		long long begin, end, step;
		char scaling[NAME_MAX];

		config_setting_t * array = config_lookup(config, FLD_STREAMARRAY_GROUP);
		config_setting_lookup_int64(array, FLD_STREAMARRAY_BEGINLENGTH, &begin);
		config_setting_lookup_int64(array, FLD_STREAMARRAY_ENDLENGTH, &end);
		config_setting_lookup_int64(array, FLD_STREAMARRAY_INCREMENT, &step);
		c2o_strncpy(scaling,
				config_setting_get_member(array, FLD_STREAMARRAY_SCALING), NAME_MAX);

		sa_opt = (struct options_streamarray) {
			(walking_t)begin,
				(walking_t)end,
				(walking_t)step
		};
	}

	// flops array group
	struct options_flopsarray fa_opt;
	{
		int length;

		config_setting_t * array = config_lookup(config, FLD_FLOPSARRAY_GROUP);
		config_setting_lookup_int(array, FLD_FLOPSARRAY_LENGTH, &length);

		fa_opt = (struct options_flopsarray) {
			length
		};
	}

	options->generic = gn_opt;
	options->walkArray = wa_opt;
	options->streamArray = sa_opt;
	options->flopsArray = fa_opt;
}

static void options_help(const char * name) {
	fprintf(stderr, "Usage: %s [options]\noptions:\n"
			"\t-h -?     print this help message\n"
			"\t-c <file> read configuration from <file>\n"
			"\t-g        print default configuration file to stdout and quit\n"
			"\t-i        print run configuration at program startup\n",
			name);
}

void options_generic_print(
		FILE * out,
		const char * prefix,
		const struct options_generic * gn_opt)
{
	fprintf(out,
			"%sprocess creation = %s;\n"
			"%sCPU frequency = %lf;\n"
			"%sCSV logging = %s;\n"
			"%sbasename for CSV logfiles = %s;\n"
			"%sprocesses = %u;\n"
			"%sthreads = %u;\n"
			"%ssilent mode = %s;\n"
			,
			prefix, spawn_typeToString(gn_opt->create),
			prefix, gn_opt->frequency,
			prefix, bool2onoff(gn_opt->logging),
			prefix, gn_opt->csvlogname,
			prefix, gn_opt->processes,
			prefix, gn_opt->threads,
			prefix, bool2onoff(gn_opt->silent)
			);
}

void options_walkarray_print(
		FILE * out,
		const char * prefix,
		const struct options_walkarray * wa_opt)
{
	fprintf(out,
			"%sbegin size = %"PRIWALKING";\n"
			"%send size = %"PRIWALKING";\n"
			"%sstep size = %"PRIWALKING";\n"
			"%saccesses = %u;\n"
			"%stest repetitions = %u;\n"
			"%scycle pattern = %s;\n"
			,
			prefix, wa_opt->begin,
			prefix, wa_opt->end,
			prefix, wa_opt->step,
			prefix, wa_opt->aaccesses,
			prefix, wa_opt->repetitions,
			prefix, pattern_typeToString(wa_opt->pattern)
			);
}

void options_streamarray_print(
		FILE * out,
		const char * prefix,
		const struct options_streamarray * sa_opt)
{
	fprintf(out,
			"%sbegin size = %zd;\n"
			"%send size = %zd;\n"
			"%sstep size = %zd;\n"
			,
			prefix, sa_opt->begin,
			prefix, sa_opt->end,
			prefix, sa_opt->step
			);
}

void options_flopsarray_print(
		FILE * out,
		const char * prefix,
		const struct options_flopsarray * fa_opt)
{
	fprintf(out,
			"%slength = %d;\n"
			,
			prefix, fa_opt->length
			);
}

void options_print(
		FILE * out,
		const char * prefix,
		const struct options * opt)
{
	// keep 'prefix' as the first characters
	const char indent[] = "  ";
	const size_t childprefixlen = 1 + strlen(prefix) + strlen(indent);
	char * childprefix = malloc(childprefixlen);
	snprintf(childprefix, childprefixlen, "%s%s", prefix, indent);

	fprintf(out, "%sgeneric :\n{\n", prefix);
	options_generic_print(out, childprefix, &(opt->generic));
	fprintf(out, "%s};\nwalk array :\n{\n", prefix);
	options_walkarray_print(out, childprefix, &(opt->walkArray));
	fprintf(out, "%s};\nstream array :\n{\n", prefix);
	options_streamarray_print(out, childprefix, &(opt->streamArray));
	fprintf(out, "%s};\nflops array :\n{\n", prefix);
	options_flopsarray_print(out, childprefix, &(opt->flopsArray));
	fprintf(out, "%s};\n", prefix);

	free(childprefix);
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
		options_print(stderr, "", options);
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
