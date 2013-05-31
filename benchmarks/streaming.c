#include "streaming.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void initStreamArray(struct streamArray * array) {
#define DO_INIT_STREAM(TYPE, ARRAY, UTYPE) do { \
	long dis_idx; TYPE dis_init; \
	for (dis_idx = 0, dis_init = 0; dis_idx < ARRAY->len; ++dis_idx, ++dis_init) { \
		ARRAY->in.UTYPE[dis_idx] = dis_init; \
		ARRAY->out.UTYPE[dis_idx] = 0; \
	}} while(0)
	switch (array->width) {
		case I8:
			DO_INIT_STREAM(int8_t, array, i8);
			break;
		case I16:
			DO_INIT_STREAM(int16_t, array, i16);
			break;
		case I32:
			DO_INIT_STREAM(int32_t, array, i32);
			break;
		case I64:
			DO_INIT_STREAM(int64_t, array, i64);
			break;
	}
}

static void allocStreamArray(struct streamArray * array) {
	array->size = array->len;
	unsigned align;
	void * in, * out;
	switch (array->width) {
		case I8:
			array->size *= sizeof(int8_t);
			align = __alignof(int8_t);
			in = &(array->in.i8);
			out = &(array->out.i8);
			break;
		case I16:
			array->size *= sizeof(int16_t);
			align = __alignof(int16_t);
			in = &(array->in.i16);
			out = &(array->out.i16);
			break;
		case I32:
			array->size *= sizeof(int32_t);
			align = __alignof(int32_t);
			in = &(array->in.i32);
			out = &(array->out.i32);
			break;
		case I64:
			array->size *= sizeof(int64_t);
			align = __alignof(int64_t);
			in = &(array->in.i64);
			out = &(array->out.i64);
			break;
		default:
			fprintf(stderr,
					"Internal error: unknown precision type requested in %s\n",
					__func__);
			exit(EXIT_FAILURE);
	}

	posix_memalign(in, align, array->size);
	posix_memalign(out, align, array->size);

	initStreamArray(array);
}

void makeStreamArray(
		enum stream_width width,
		unsigned len,
		struct streamArray ** result)
{
	*result = malloc(sizeof(struct streamArray));

	(*result)->width = width;
	(*result)->len = len;

	allocStreamArray(*result);
}

void freeStreamArray(struct streamArray * array) {
	switch (array->width) {
		case I8:
			free(array->in.i8);
			free(array->out.i8);
			break;
		case I16:
			free(array->in.i16);
			free(array->out.i16);
			break;
		case I32:
			free(array->in.i32);
			free(array->out.i32);
			break;
		case I64:
			free(array->in.i64);
			free(array->out.i64);
			break;
	}
	free(array);
}

void streamArray(struct streamArray * array) {
#define DO_STREAM(ARRAY, UTYPE) do { \
	long ds_idx; \
	for (ds_idx = 0; ds_idx < ARRAY->len; ++ds_idx) { \
		ARRAY->out.UTYPE[ds_idx] = ARRAY->in.UTYPE[ds_idx]; \
	}} while(0)
	switch (array->width) {
		case I8:
			DO_STREAM(array, i8);
			break;
		case I16:
			DO_STREAM(array, i16);
			break;
		case I32:
			DO_STREAM(array, i32);
			break;
		case I64:
			DO_STREAM(array, i64);
			break;
	}
}

void memcpyArray(struct streamArray * array) {
	switch (array->width) {
		case I8:
			memcpy(array->out.i8, array->in.i8, array->size);
			break;
		case I16:
			memcpy(array->out.i16, array->in.i16, array->size);
			break;
		case I32:
			memcpy(array->out.i32, array->in.i32, array->size);
			break;
		case I64:
			memcpy(array->out.i64, array->in.i64, array->size);
			break;
	}
}
