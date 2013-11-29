#include "flops.hpp"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fvec_init(int len, float * vec) {
	int idx;
	float init;
	for (idx = 0, init = 0; idx < len; ++idx, ++init)
		vec[idx] = init;
}

static void dvec_init(int len, double * vec) {
	int idx;
	double init;
	for (idx = 0, init = 0; idx < len; ++idx, ++init)
		vec[idx] = init;
}

// aligned allocation to prevent crossing cache line boundaries for data that
// fits on a single cache line
static void allocFlopsArray(struct FlopsArray * array) {
	size_t align;
	void * data, * scale, * offset;

	array->size = (size_t)array->len;
	switch (array->precision) {
		case SINGLE:
			array->size *= sizeof(float);
			align = __alignof(float);
			data = &(array->vec.sp.data);
			scale = &(array->vec.sp.scale);
			offset = &(array->vec.sp.offset);
			break;
		case DOUBLE:
			array->size *= sizeof(double);
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

	if (align < sizeof(void*))
		align = sizeof(void*);

	int ret;
	if (0 != (ret = posix_memalign(&data, align, array->size)))
		fprintf(stderr, "data memory allocation failed: %s\n", strerror(ret));
	if (0 != (ret = posix_memalign(&scale, align, array->size)))
		fprintf(stderr, "scale memory allocation failed: %s\n", strerror(ret));
	if (0 != (ret = posix_memalign(&offset, align, array->size)))
		fprintf(stderr, "offset memory allocation failed: %s\n", strerror(ret));

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
		struct FlopsArray ** result)
{
	*result = static_cast<FlopsArray *>(malloc(sizeof(struct FlopsArray)));

	(*result)->precision = precision;
	(*result)->len = len;
	allocFlopsArray(*result);
}

void freeFlopsArray(struct FlopsArray * array) {
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

float flops_madd16(float init, const long long iterations) {
	float a[] = {init, init + 2, init + 4, init + 8};
	const float c = 4.2f;
	const float x = 0.9f;
	for (long iter = 0; iter < iterations; ++iter) {
		a[0] = a[0] * x + c;
		a[1] = a[1] * x + c;
		a[2] = a[2] * x + c;
		a[3] = a[3] * x + c;

		a[0] = a[0] * x + c;
		a[1] = a[1] * x + c;
		a[2] = a[2] * x + c;
		a[3] = a[3] * x + c;

		a[0] = a[0] * x + c;
		a[1] = a[1] * x + c;
		a[2] = a[2] * x + c;
		a[3] = a[3] * x + c;

		a[0] = a[0] * x + c;
		a[1] = a[1] * x + c;
		a[2] = a[2] * x + c;
		a[3] = a[3] * x + c;
	}

	return a[0] + a[1] + a[2] + a[3];
}

typedef float fvec_fn(const int, float * restrict, const long long);
typedef double dvec_fn(const int, double * restrict, const long long);

static const float fc = 4.2f;
static const float fx = 0.9f;
static const double dc = 4.2;
static const double dx = 0.9;

// for some currently unknown reason, icc refrains from vectorizing this loop
// when length is of unsigned type; adding the ivdep pragma is another
// solution to this problem, which allows unsigned length ... ehm ... ?
static float fvec_add(const int len, float * src, const long long iterations) {
	for (long iter = 0; iter < iterations; ++iter)
		for (int idx = 0; idx < len; ++idx)
			src[idx] = src[idx] + fc;
	return *src;
}

static float fvec_mul(const int len, float * src, const long long iterations) {
	for (long iter = 0; iter < iterations; ++iter)
		for (int idx = 0; idx < len; ++idx)
			src[idx] = src[idx] * fc;
	return *src;
}

static float fvec_madd(const int len, float * src, const long long iterations) {
	for (long iter = 0; iter < iterations; ++iter)
		for (int idx = 0; idx < len; ++idx)
			src[idx] = src[idx] * fx + fc;
	return *src;
}

static double dvec_add(const int len, double * src, const long long iterations) {
	for (long iter = 0; iter < iterations; ++iter)
		for (int idx = 0; idx < len; ++idx)
			src[idx] = src[idx] + dc;
	return *src;
}

static double dvec_mul(const int len, double * src, const long long iterations) {
	for (long iter = 0; iter < iterations; ++iter)
		for (int idx = 0; idx < len; ++idx)
			src[idx] = src[idx] * dc;
	return *src;
}

static double dvec_madd(const int len, double * src, const long long iterations) {
	for (long iter = 0; iter < iterations; ++iter)
		for (int idx = 0; idx < len; ++idx)
			src[idx] = src[idx] * dx + dc;
	return *src;
}

double flopsArray(enum flop_t operation, struct FlopsArray * array, unsigned long long iterations) {
	double result = 0;
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
				result = func(array->len, array->vec.sp.data, (long long)iterations);
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
				result = func(array->len, array->vec.dp.data, (long long)iterations);
			}
			break;
	}
	return result;
}
