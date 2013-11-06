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

struct StreamArray {
	enum stream_width width;
	unsigned len;
	size_t size;
	array_t in;
	array_t out;
};

void makeStreamArray(
		enum stream_width width,
		unsigned len,
		struct StreamArray ** result);

void freeStreamArray(struct StreamArray * array);

void streamArray(struct StreamArray * array);

void memcpyArray(struct StreamArray * array);

void fillArray(struct StreamArray * array);

void memsetArray(struct StreamArray * array);
