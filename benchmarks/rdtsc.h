#pragma once

#include<stdint.h>

#if defined(__i386__)

static inline uint64_t rdtsc(void) {
	uint64_t stamp;
	asm volatile ("rdtsc":"=A"(stamp));
	return stamp;
}

#elif defined(__x86_64__)

static inline uint64_t rdtsc(void) {
	uint32_t lo, hi;
	asm volatile ("rdtsc":"=a"(lo),"=d"(hi));
	return lo | ((uint64_t) hi << 32);
}

#endif
