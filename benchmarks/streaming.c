#include "streaming.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

void makeStreamArray(
		enum stream_width width,
		size_t len,
		struct streamArray ** result)
{
	*result = malloc(sizeof(struct streamArray));

	(*result)->width = width;
	(*result)->len = len;

	switch (width) {
		case I8:
			(*result)->in.i8 = malloc(len * sizeof(int8_t));
			(*result)->out.i8 = malloc(len * sizeof(int8_t));
			{
				size_t idx;
				int8_t init;
				for (idx = 0, init = 0; idx < len; ++idx, ++init) {
					(*result)->in.i8[idx] = init;
					(*result)->out.i8[idx] = 0;
				}
			}
			break;
		case I16:
			(*result)->in.i16 = malloc(len * sizeof(int16_t));
			(*result)->out.i16 = malloc(len * sizeof(int16_t));
			{
				size_t idx;
				int16_t init;
				for (idx = 0, init = 0; idx < len; ++idx, ++init) {
					(*result)->in.i16[idx] = init;
					(*result)->out.i16[idx] = 0;
				}
			}
			break;
		case I32:
			(*result)->in.i32 = malloc(len * sizeof(int32_t));
			(*result)->out.i32 = malloc(len * sizeof(int32_t));
			{
				size_t idx;
				int32_t init;
				for (idx = 0, init = 0; idx < len; ++idx, ++init) {
					(*result)->in.i32[idx] = init;
					(*result)->out.i32[idx] = 0;
				}
			}
			break;
		case I64:
			(*result)->in.i64 = malloc(len * sizeof(int64_t));
			(*result)->out.i64 = malloc(len * sizeof(int64_t));
			{
				size_t idx;
				int64_t init;
				for (idx = 0, init = 0; idx < len; ++idx, ++init) {
					(*result)->in.i64[idx] = init;
					(*result)->out.i64[idx] = 0;
				}
			}
			break;
	}
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
	switch (array->width) {
		case I8:
			for (size_t idx = 0; idx < array->len; ++idx)
				array->out.i8[idx] = array->in.i8[idx];
			break;
		case I16:
			for (size_t idx = 0; idx < array->len; ++idx)
				array->out.i16[idx] = array->in.i16[idx];
			break;
		case I32:
			for (size_t idx = 0; idx < array->len; ++idx)
				array->out.i32[idx] = array->in.i32[idx];
			break;
		case I64:
			for (size_t idx = 0; idx < array->len; ++idx)
				array->out.i64[idx] = array->in.i64[idx];
			break;
	}
}
