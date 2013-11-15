#pragma once

// TODO libconfig as backend

#include <cstddef>
#include <cstdint>

namespace arraywalk {

	enum pattern { RANDOM, INCREASING, DECREASING };

	namespace defaults {
		static constexpr size_t size_min = 1 << 12;
		static constexpr size_t size_max = 1 << 14;
		static constexpr unsigned size_mul = 1;
		static constexpr size_t size_inc = 1 << 12;

		static constexpr unsigned istream_min = 1;
		static constexpr unsigned istream_max = 5;

		static constexpr uintptr_t align = 1 << 12;

		static constexpr pattern ptrn = RANDOM;

		static constexpr uint_fast32_t MiB = 1 << 8;
	}

	struct Config/*: public libconfig::Config */ {
		Config(
				size_t _size_min      = defaults::size_min,
				size_t _size_max      = defaults::size_max,
				unsigned _size_mul    = defaults::size_mul,
				size_t _size_inc      = defaults::size_inc,
				unsigned _istream_min = defaults::istream_min,
				unsigned _istream_max = defaults::istream_max,
				size_t _align         = defaults::align,
				pattern _ptrn         = defaults::ptrn,
				uint_fast32_t _MiB    = defaults::MiB);

		size_t size_min;
		size_t size_max;
		unsigned size_mul;
		size_t size_inc;
		unsigned istream_min;
		unsigned istream_max;
		uintptr_t align;
		pattern ptrn;
		uint_fast32_t MiB;
	};
}
