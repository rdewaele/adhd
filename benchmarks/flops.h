#pragma once

enum floating_t {SINGLE, DOUBLE};
enum flop_t {ADD,MUL,MADD};

struct flopsArray {
	enum floating_t precision;
	int len; // needs to be signed for icc to vectorize ...
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
