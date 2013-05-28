#include "flops.h"

void sp_2fma(float init, int iterations) {
	float a = init;
	float b = init * 2;

	float x = init;
	float c = init + 2;
#pragma simd
	for (int i = 0; i < iterations; ++i) {
	  a = a*x + c;
		b = b*x + c;
	}
}
