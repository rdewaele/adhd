#pragma once

#include "benchmarks.hpp"

#include <libconfig.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

enum spawn_type {LINEAR, TREE};

const char * spawn_typeToString(enum spawn_type st);
enum spawn_type spawn_typeFromString(const char * st);

const char * pattern_typeToString(enum pattern_type pt);
enum pattern_type pattern_typeFromString(const char * pt);

struct Options_generic {
	// process Creation
	enum spawn_type create;
	// cpu frequency to use in memory access cycles calculation
	double frequency;
	// CSV logging enable/disable
	bool logging;
	// base filename for csv files (pid will be appended)
	char csvlogname[NAME_MAX];
	// number of processes to run in parallel: begin
	unsigned processes_begin;
	// number of processes to run in parallel: end
	unsigned processes_end;
	// silent mode
	bool silent;
	// number of threads to run per process: begin
	unsigned threads_begin;
	// number of threads to run per process: end
	unsigned threads_end;
};

struct Options_walkarray {
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

struct Options_streamarray {
	// initial size for the benchmark's array
	unsigned begin;
	// upper size limit for the benchmark's array
	unsigned end;
	// the test array grows linearly by this amount (determines granularity)
	unsigned step;
};

struct Options_flopsarray {
	// initial size for the benchmark's array
	unsigned begin;
	// upper size limit for the benchmark's array
	unsigned end;
	// number of calculations to perform in tests
	long long unsigned calculations;
	// the test array grows linearly by this amount (determines granularity)
	unsigned step;
};

struct Options {
	struct Options_generic generic;
	struct Options_walkarray walkArray;
	struct Options_streamarray streamArray;
	struct Options_flopsarray flopsArray;
};

void options_parse(int argc, char * argv[], struct Options * options);

void options_generic_print(
		FILE * out,
		const char * prefix,
		const struct Options_generic * gn_opt);

void options_walkarray_print(
		FILE * out,
		const char * prefix,
		const struct Options_walkarray * wa_opt);

void options_streamarray_print(
		FILE * out,
		const char * prefix,
		const struct Options_streamarray * sa_opt);

void options_flopsarray_print(
		FILE * out,
		const char * prefix,
		const struct Options_flopsarray * fa_opt);

void options_print(
		FILE * out,
		const char * prefix,
		const struct Options * opt);
