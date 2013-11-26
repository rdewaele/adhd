#include "config.hpp"

#include <cstddef>
#include <cstdint>

// TODO type conversions: bounds checking
namespace reduction {
	Config::Config( size_t _size_min, size_t _size_max, unsigned _size_mul, size_t _size_inc,
			unsigned _istream_min, unsigned _istream_max,
			uintptr_t _align, pattern _ptrn, uint_fast32_t _MiB):
		size_min(_size_min),
		size_max(_size_max),
		size_mul(_size_mul),
		size_inc(_size_inc),
		istream_min(_istream_min),
		istream_max(_istream_max),
		align(_align),
		ptrn(_ptrn),
		MiB(_MiB)
	{
		// TODO: argument validity checks
	}
}
