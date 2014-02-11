#include "config.hpp"

#include <cstddef>
#include <cstdint>

// TODO type conversions: bounds checking
namespace arraywalk {
	using namespace adhd;

	Config::Config( unsigned _threads_min, unsigned _threads_max,
			size_t _size_min, size_t _size_max, unsigned _size_mul,
			size_t _size_inc, unsigned _istream_min, unsigned _istream_max,
			uintptr_t _align, pattern _ptrn, uint_fast32_t _MiB):
		RangeSet(
				ArraySize(_size_min, _size_max, _size_inc, _size_mul),
				Range<unsigned>(_istream_min, _istream_max)),
		threads_min(_threads_min),
		threads_max(_threads_max),
		align(_align),
		ptrn(_ptrn),
		readMiB(_MiB)
	{
		// TODO: argument validity checks
	}
}
