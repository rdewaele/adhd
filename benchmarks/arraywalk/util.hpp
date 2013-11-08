#pragma once

#include <type_traits>

namespace util {
	template <typename T>
	inline bool isPowerOfTwo(T x) {
		return std::is_integral<T>::value && ((x & (x - 1)) == 0);
	}
}
