#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef WALKING_MIN_WIDTH
// default to a minimum width of 32 bits;
// assuming CHAR_BIT = 8, 16 bits would yield a maximal array size of
// 2 * 2^16 bytes = 128KiB, which is smaller than most caches today
// (32 bits should be plenty for quite a while: 4 * 2^32 bytes = 16GiB)
#define WALKING_MIN_WIDTH 32
#endif

// Configure the type of the elements of walkarrays.
#if WALKING_MIN_WIDTH == 8
typedef uint_fast8_t walking_t;
#define PRIWALKING PRIuFAST8
#define WALKING_MAX UINT_FAST8_MAX
#elif WALKING_MIN_WIDTH == 16
typedef uint_fast16_t walking_t;
#define PRIWALKING PRIuFAST16
#define WALKING_MAX UINT_FAST16_MAX
#elif WALKING_MIN_WIDTH == 32
typedef uint_fast32_t walking_t;
#define PRIWALKING PRIuFAST32
#define WALKING_MAX UINT_FAST32_MAX
#elif WALKING_MIN_WIDTH == 64
typedef uint_fast64_t walking_t;
#define PRIWALKING PRIuFAST64
#define WALKING_MAX UINT_FAST64_MAX
#else // no valid min width
#error "no valid min width for walking array type"
#endif

#ifndef NDEBUG
#if WALKING_MAX == UINT8_MAX
#pragma message "sizeof walking_t == 1"
#elif WALKING_MAX == UINT16_MAX
#pragma message "sizeof walking_t == 2"
#elif WALKING_MAX == UINT32_MAX
#pragma message "sizeof walking_t == 4"
#elif WALKING_MAX == UINT64_MAX
#pragma message "sizeof walking_t == 8"
#endif
#endif

// arithmetic operations promote their arguments to int; casting makes our
// intention explicit, when walking_t has a smaller type than int
#define WALKING_T_CAST(EXP) (walking_t)(EXP)

// Walkarrays know their length. (And for convenience, their size in bytes.)
struct WalkArray {
	walking_t len;
	size_t size;
	walking_t * array;
};

// Encode a random generated cyclic path of given length in an array.
struct timespec makeRandomWalkArray(walking_t len, struct WalkArray ** result);

// Encode a linear increasing cyclic path of given length in an array.
struct timespec makeIncreasingWalkArray(walking_t len, struct WalkArray ** result);

// Encode a linear decreasing cyclic path of given length in an array.
struct timespec makeDecreasingWalkArray(walking_t len, struct WalkArray ** result);

// pattern_type maps to the different kinds of walk arrays that can be created
enum pattern_type {RANDOM, INCREASING, DECREASING};

// Encode a cyclic path of given type and length in an array.
struct timespec makeWalkArray(
		enum pattern_type p,
		walking_t len,
		struct WalkArray ** result);

// Free the walkArray structure.
void freeWalkArray(struct WalkArray * array);

// Walk the array as encoded by makeRandomWalkArray(size_t len), store timing
// information in 'elapsed' (must be valid), and return final index.
walking_t walkArray(struct WalkArray * array, size_t steps, struct timespec * elapsed);

// Verify whether the input array implements a single cycle of a lengh equal
// to the array itself.
// Note that the make*WalkArray functions always return cycles conform to this
// specification. (If they aren't, it's a bug.)
bool isFullCycle(walking_t * array, walking_t len);
