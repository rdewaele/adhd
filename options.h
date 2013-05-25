#pragma once

#include "arraywalk.h"

#include <libconfig.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

enum spawn_type {LINEAR, TREE};
enum pattern_type {RANDOM, INCREASING, DECREASING};

const char * spawn_typeToString(enum spawn_type st);
enum spawn_type spawn_typeFromString(const char * st);

const char * pattern_typeToString(enum pattern_type pt);
enum pattern_type pattern_typeFromString(const char * pt);

struct options {
	// amount of array accesses to perform (improves bench accuracy)
	unsigned aaccesses;
	// initial size for the benchmark's array
	walking_t begin;
	// process Creation
	enum spawn_type create;
	// upper size limit for the benchmark's array
	walking_t end;
	// cpu frequency to use in memory access cycles calculation
	double frequency;
	// CSV logging enable/disable
	bool logging;
	// base filename for csv files (pid will be appended)
	char csvlogname[NAME_MAX];
	// walking pattern: traverse an array sequentially (up/down) or randomly
	enum pattern_type pattern;
	// number of processes to run in parallel
	unsigned processes;
	// repeats for each test case (increases test significance)
	unsigned repetitions;
	// the test array grows linearly by this amount (determines granularity)
	walking_t step;
	// silent mode
	bool Silent;
	// number of threads to run per process
	unsigned threads;
};

void options_parse(int argc, char * argv[], struct options * options);
