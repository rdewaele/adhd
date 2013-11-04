#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "c_bindings.h"

static void
report(size_t idx_size, uint64_t cycles, uint64_t reads)
{
	printf("cycles: %"PRIu64"\n", cycles);
	printf("total reads: %"PRIu64" (%"PRIu64" bytes)\n", reads, reads * idx_size);
	printf("~cycles per read: %lf\n", (double)cycles / (double)reads);
}

static void
run_test(size_t arraysize, uint_fast32_t loops)
{
	uint64_t cycles = 0;
	uint64_t reads = 0;
	void * test = aw_alloc(AW64, arraysize / sizeof(uint64_t), RANDOM);
	aw_timedwalk_loc(test, 1, loops, &cycles, &reads);
	report(sizeof(uint64_t), cycles, reads);
	aw_free(test);
}

int
main()
{
	size_t arraysize;
	for (arraysize = 1ul << 7; arraysize < 1ul << 16; arraysize *= 2) {
		const uint32_t loops = 1 << 6;
		run_test(arraysize, loops);
	}
	return 0;
}
