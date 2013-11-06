#pragma once

#include <stddef.h>

enum floating_t {SINGLE, DOUBLE};
enum flop_t {ADD,MUL,MADD};

struct FlopsArray {
	enum floating_t precision;
	int len; // needs to be signed for icc to vectorize ...
	size_t size;
	union {
		struct {
			float * data;
			float * scale;
			float * offset;
		} sp;
		struct {
			double * data;
			double * scale;
			double * offset;
		} dp;
	} vec;
};

void makeFlopsArray(
		enum floating_t precision,
		int len,
		struct FlopsArray ** result);

void freeFlopsArray(struct FlopsArray * array);

double flopsArray(enum flop_t operation, struct FlopsArray * array, unsigned long long calculations);

float flops_madd16(float init, const long long iterations);
