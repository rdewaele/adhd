#include <cstdint>
#include <type_traits>

#include "c_bindings.h"
#include "arraywalk.hpp"

using namespace std;
using namespace arraywalk;

namespace arraywalk {
	class AWClassPtr {
		public:
			AWClassPtr(enum awTypeWidth awtw, size_t el, enum pattern pt):
				tw(awtw)
			{
				// C-style casts: icpc (but not gcc) issues an implicit conversion warning
				// when using static_cast (why?)
				switch (tw) {
					case AW8:
						cp = new ArrayWalk<uint8_t>((uint8_t)el, pt);
						break;
					case AW16:
						cp = new ArrayWalk<uint16_t>((uint16_t)el, pt);
						break;
					case AW32:
						cp = new ArrayWalk<uint32_t>((uint32_t)el, pt);
						break;
					case AW64:
						cp = new ArrayWalk<uint64_t>((uint64_t)el, pt);
						break;
					case AW128:
						cp = new ArrayWalk<__uint128_t>((__uint128_t)el, pt);
						break;
				}
			}

			~AWClassPtr(void)
			{
				switch (tw) {
					case AW8:
						delete static_cast<ArrayWalk<uint8_t> *>(cp);
						break;
					case AW16:
						delete static_cast<ArrayWalk<uint16_t> *>(cp);
						break;
					case AW32:
						delete static_cast<ArrayWalk<uint32_t> *>(cp);
						break;
					case AW64:
						delete static_cast<ArrayWalk<uint64_t> *>(cp);
						break;
					case AW128:
						delete static_cast<ArrayWalk<__uint128_t> *>(cp);
						break;
				}
			}

			void timedwalk_loc(unsigned locs, uint_fast32_t loops, uint64_t & cycles, uint64_t & reads)
			{
				switch (tw) {
					case AW8:
						static_cast<ArrayWalk<uint8_t> *>(cp)->timedwalk_loc(locs, loops, cycles, reads);
						break;
					case AW16:
						static_cast<ArrayWalk<uint16_t> *>(cp)->timedwalk_loc(locs, loops, cycles, reads);
						break;
					case AW32:
						static_cast<ArrayWalk<uint32_t> *>(cp)->timedwalk_loc(locs, loops, cycles, reads);
						break;
					case AW64:
						static_cast<ArrayWalk<uint64_t> *>(cp)->timedwalk_loc(locs, loops, cycles, reads);
						break;
					case AW128:
						static_cast<ArrayWalk<__uint128_t> *>(cp)->timedwalk_loc(locs, loops, cycles, reads);
						break;
					}
			}

		private:
			void * cp;
			const enum awTypeWidth tw;
	};
}

extern "C" {
	void *
	aw_alloc(enum awTypeWidth awtw, size_t elem, enum pattern kind)
	{
		return new (nothrow) AWClassPtr(awtw, elem, kind);
	}

	void
	aw_timedwalk_loc(void * awcp, unsigned locs, uint_fast32_t loops,
	                 uint64_t * cycles, uint64_t * reads)
	{
		static_cast<AWClassPtr *>(awcp)->timedwalk_loc(locs, loops, *cycles, *reads);
	}

	void
	aw_free(void * awcp) {
		delete static_cast<AWClassPtr *>(awcp);
	}
}
