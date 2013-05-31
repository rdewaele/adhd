#pragma once

#include "benchmarks.h"

#include <libconfig.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

enum spawn_type {LINEAR, TREE};

const char * spawn_typeToString(enum spawn_type st);
enum spawn_type spawn_typeFromString(const char * st);

const char * pattern_typeToString(enum pattern_type pt);
enum pattern_type pattern_typeFromString(const char * pt);

struct options_generic {
	// process Creation
	enum spawn_type create;
	// cpu frequency to use in memory access cycles calculation
	double frequency;
	// CSV logging enable/disable
	bool logging;
	// base filename for csv files (pid will be appended)
	char csvlogname[NAME_MAX];
	// number of processes to run in parallel
	unsigned processes;
	// silent mode
	bool silent;
	// number of threads to run per process
	unsigned threads;
};

struct options_walkarray {
	// amount of array accesses to perform (improves bench accuracy)
	unsigned aaccesses;
	// initial size for the benchmark's array
	walking_t begin;
	// upper size limit for the benchmark's array
	walking_t end;
	// walking pattern: traverse an array sequentially (up/down) or randomly
	enum pattern_type pattern;
	// calculate average and standard deviation based on this number of runs
	unsigned repetitions;
	// the test array grows linearly by this amount (determines granularity)
	walking_t step;
};

struct options_streamarray {
	// initial size for the benchmark's array
	unsigned begin;
	// upper size limit for the benchmark's array
	unsigned end;
	// the test array grows linearly by this amount (determines granularity)
	unsigned step;
};

struct options_flopsarray {
	// size for the array of floating types
	int length;
};

struct options {
	struct options_generic generic;
	struct options_walkarray walkArray;
	struct options_streamarray streamArray;
	struct options_flopsarray flopsArray;
};

void options_parse(int argc, char * argv[], struct options * options);

void options_generic_print(
		FILE * out,
		const char * prefix,
		const struct options_generic * gn_opt);

void options_walkarray_print(
		FILE * out,
		const char * prefix,
		const struct options_walkarray * wa_opt);

void options_streamarray_print(
		FILE * out,
		const char * prefix,
		const struct options_streamarray * sa_opt);

void options_flopsarray_print(
		FILE * out,
		const char * prefix,
		const struct options_flopsarray * fa_opt);

void options_print(
		FILE * out,
		const char * prefix,
		const struct options * opt);
