#pragma once

#include "../benchmark.hpp"

// TODO libconfig as backend

#include <cstddef>
#include <cstdint>

#include <iostream>

namespace arraywalk {

	enum class Pattern { RANDOM, INCREASING, INCREASING_MAXSTRIDE, DECREASING };

	using CAS_arraysize = adhd::AffineStepper<size_t>;
	using CAS_istreams = adhd::AffineStepper<unsigned>;
	using CAS_alignment = adhd::AffineStepper<uintptr_t>;

	namespace defaults {
		static constexpr unsigned threads_min = 1;
		static constexpr unsigned threads_max = 4;

		static constexpr size_t size_min = 1 << 12;
		static constexpr size_t size_max = 1 << 14;
		static constexpr size_t size_mul = 1;
		static constexpr size_t size_inc = 1 << 12;

		static constexpr unsigned istream_min = 1;
		static constexpr unsigned istream_max = 5;

		static constexpr uintptr_t align_min = 1 << 12;
		static constexpr uintptr_t align_max = 1 << 30;
		static constexpr uintptr_t align_mul = 2;
		static constexpr uintptr_t align_inc = 0;

		static constexpr Pattern ptrn = Pattern::RANDOM;

		static constexpr uint_fast32_t MiB = 1 << 8;
	}

	struct Config: public adhd::RangeSet<CAS_arraysize, CAS_istreams, CAS_alignment> {

		Config(
				unsigned _threads_min = defaults::threads_min,
				unsigned _threads_max = defaults::threads_max,
				size_t _size_min      = defaults::size_min,
				size_t _size_max      = defaults::size_max,
				unsigned _size_mul    = defaults::size_mul,
				size_t _size_inc      = defaults::size_inc,
				unsigned _istream_min = defaults::istream_min,
				unsigned _istream_max = defaults::istream_max,
				uintptr_t _align_min  = defaults::align_min,
				uintptr_t _align_max  = defaults::align_max,
				uintptr_t _align_mul  = defaults::align_mul,
				uintptr_t _align_inc  = defaults::align_inc,
				Pattern _ptrn         = defaults::ptrn,
				uint_fast32_t _MiB    = defaults::MiB);

		size_t inline minSize() const { return getMinValue<0>(); }
		size_t inline maxSize() const { return getMaxValue<0>(); }
		size_t inline currentSize() const { return getValue<0>(); }

		unsigned inline minIStream() const { return getMinValue<1>(); }
		unsigned inline maxIStream() const { return getMaxValue<1>(); }
		unsigned inline currentIStream() const { return getValue<1>(); }

		uintptr_t inline minAlign() const { return getMinValue<2>(); }
		uintptr_t inline maxAlign() const { return getMaxValue<2>(); }
		uintptr_t inline currentAlign() const { return getValue<2>(); }

		unsigned threads_min;
		unsigned threads_max;
		Pattern ptrn;
		uint_fast32_t readMiB;
	};
}
