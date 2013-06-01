#pragma once

#include <stddef.h>
#include <stdint.h>

enum stream_width {I8, I16, I32, I64};

typedef union {
	int8_t * i8;
	int16_t * i16;
	int32_t * i32;
	int64_t * i64;
} array_t;

struct streamArray {
	enum stream_width width;
	unsigned len;
	size_t size;
	array_t in;
	array_t out;
};

void makeStreamArray(
		enum stream_width width,
		unsigned len,
		struct streamArray ** result);

void freeStreamArray(struct streamArray * array);

void streamArray(struct streamArray * array);

void memcpyArray(struct streamArray * array);

void fillArray(struct streamArray * array);
