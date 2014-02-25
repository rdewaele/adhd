#include "config.hpp"

#include <cstddef>
#include <cstdint>

// TODO type conversions: bounds checking
namespace arraywalk {
	using namespace adhd;

	Config::Config( unsigned _threads_min, unsigned _threads_max,
			size_t _size_min, size_t _size_max, unsigned _size_mul,
			size_t _size_inc, unsigned _istream_min, unsigned _istream_max,
			uintptr_t _align_min, uintptr_t _align_max, uintptr_t _align_mul,
			uintptr_t _align_inc, Pattern _ptrn, uint_fast32_t _MiB):
		RangeSet(
				CAS_arraysize(_size_min, _size_max, _size_mul, _size_inc),
				CAS_istreams(_istream_min, _istream_max),
				CAS_alignment(_align_min, _align_max, _align_mul, _align_inc)),
		threads_min(_threads_min),
		threads_max(_threads_max),
		ptrn(_ptrn),
		readMiB(_MiB)
	{
		// TODO: argument validity checks
	}
}
