#pragma once

#include "arraywalk.h"

#include <libconfig.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

enum spawn_type {LINEAR, TREE};

const char * spawn_typeToString(enum spawn_type st);
enum spawn_type spawn_typeFromString(const char * st);

struct options {
	// amount of array accesses to perform (improves bench accuracy)
	unsigned aaccesses;
	// thread Creation
	enum spawn_type create;
	// upper size limit for the benchmark's array
	walking_t end;
	// cpu frequency to use in memory access cycles calculation
	double frequency;
	// CSV logging enable/disable
	bool logging;
	// base filename for csv files (pid will be appended)
	char csvlogname[NAME_MAX];
	// number of processes to run in parallel
	unsigned processes;
	// repeats for each test case (increases test significance)
	unsigned repetitions;
	// the test array grows linearly by this amount (determines granularity)
	walking_t step;
	// silent mode
	bool Silent;
};

void options_parse(int argc, char * argv[], struct options * options);
