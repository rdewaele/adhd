#pragma once

#include<stdint.h>

static inline uint64_t rdtsc(void) {
	uint32_t lo, hi;
	asm volatile ("rdtsc":"=a"(lo),"=d"(hi));
	return lo | ((uint64_t) hi << 32);
}
