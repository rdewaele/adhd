#include "flops.h"

#include <stdio.h>
#include <stdlib.h>

static void fvec_init(int len, float * restrict vec) {
	int idx;
	float init;
	for (idx = 0, init = 0; idx < len; ++idx, ++init)
		vec[idx] = init;
}

static void dvec_init(int len, double * restrict vec) {
	int idx;
	double init;
	for (idx = 0, init = 0; idx < len; ++idx, ++init)
		vec[idx] = init;
}

// aligned allocation to prevent crossing cache line boundaries for data that
// fits on a single cache line
static void allocFlopsArray(struct flopsArray * array) {
	size_t size = (size_t)array->len;
	size_t align;
	void * data, * scale, * offset;
	switch (array->precision) {
		case SINGLE:
			size *= sizeof(float);
			align = __alignof(float);
			data = &(array->vec.sp.data);
			scale = &(array->vec.sp.scale);
			offset = &(array->vec.sp.offset);
			break;
		case DOUBLE:
			size *= sizeof(double);
			align = __alignof(double);
			data = &(array->vec.dp.data);
			scale = &(array->vec.dp.scale);
			offset = &(array->vec.dp.offset);
			break;
		default:
			fprintf(stderr,
					"Internal error: unknown precision type requested in %s\n",
					__func__);
			exit(EXIT_FAILURE);
	}

	posix_memalign(data, align, size);
	posix_memalign(scale, align, size);
	posix_memalign(offset, align, size);

	switch (array->precision) {
		case SINGLE:
			fvec_init(array->len, array->vec.sp.data);
			fvec_init(array->len, array->vec.sp.scale);
			fvec_init(array->len, array->vec.sp.offset);
			break;
		case DOUBLE:
			dvec_init(array->len, array->vec.dp.data);
			dvec_init(array->len, array->vec.dp.scale);
			dvec_init(array->len, array->vec.dp.offset);
			break;
	}
}

void makeFlopsArray(
		enum floating_t precision,
		int len,
		struct flopsArray ** result)
{
	*result = malloc(sizeof(struct flopsArray));

	(*result)->precision = precision;
	(*result)->len = len;
	allocFlopsArray(*result);
}

void freeFlopsArray(struct flopsArray * array) {
	switch (array->precision) {
		case SINGLE:
			free(array->vec.sp.data);
			free(array->vec.sp.scale);
			free(array->vec.sp.offset);
			break;
		case DOUBLE:
			free(array->vec.dp.data);
			free(array->vec.dp.scale);
			free(array->vec.dp.offset);
			break;
	}
	free(array);
}

typedef void fvec_fn(const int, float * restrict, float * restrict, float * restrict);
typedef void dvec_fn(const int, double * restrict, double * restrict, double * restrict);

// for some currently unknown reason, icc refrains from vectorizing this loop
// when length is of unsigned type; adding the ivdep pragma is another
// solution to this problem, which allows unsigned length ... ehm ... ?
#define DEF_MAP_FLOP(NAME,FT,OP)\
	static void NAME (const int length,\
			FT * restrict dst,\
			FT * restrict srcA,\
			FT * restrict srcB) {\
		for (int idx = 0; idx < length; ++idx) {\
			dst[idx] = srcA[idx] OP srcB[idx];\
		}\
	}

DEF_MAP_FLOP(fvec_add,float,+)
DEF_MAP_FLOP(dvec_add,double,+)
DEF_MAP_FLOP(fvec_mul,float,*)
DEF_MAP_FLOP(dvec_mul,double,*)

#define DEF_MADD(NAME,FT)\
	static void NAME (const int length,\
			FT * restrict dst,\
			FT * restrict srcA,\
			FT * restrict srcB) {\
		for (int idx = 0; idx < length; ++idx) {\
			dst[idx] = dst[idx] * srcA[idx] + srcB[idx];\
		}\
	}

DEF_MADD(fvec_madd,float)
DEF_MADD(dvec_madd,double)

void flopsArray(enum flop_t operation, struct flopsArray * array) {
	switch (array->precision) {
		case SINGLE:
			{
				fvec_fn * func;
				switch (operation) {
					case ADD:
						func = fvec_add;
						break;
					case MUL:
						func = fvec_mul;
						break;
					case MADD:
						func = fvec_madd;
						break;
					default:
						fprintf(stderr, "Internal error: unknown operation type in %s\n", __func__);
						exit(EXIT_FAILURE);
				}
				func(array->len, array->vec.sp.data, array->vec.sp.scale, array->vec.sp.offset);
			}
			break;
		case DOUBLE:
			{
				dvec_fn * func;
				switch (operation) {
					case ADD:
						func = dvec_add;
						break;
					case MUL:
						func = dvec_mul;
						break;
					case MADD:
						func = dvec_madd;
						break;
					default:
						fprintf(stderr, "Internal error: unknown operation type in %s\n", __func__);
						exit(EXIT_FAILURE);
				}
				func(array->len, array->vec.dp.data, array->vec.dp.scale, array->vec.dp.offset);
			}
			break;
	}
}
