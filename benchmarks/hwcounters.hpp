#pragma once

#include <papi.h>

#if 1
namespace adhd {
	namespace hwcounters {
		using type = int;
		namespace CACHE {
			enum class L1: type {
				DCA = PAPI_L1_DCA, // L1 data cache accesses
				DCH = PAPI_L1_DCH, // L1 data cache hits
				DCM = PAPI_L1_DCM, // L1 data cache misses
				DCR = PAPI_L1_DCR, // L1 data cache reads
				DCW = PAPI_L1_DCW, // L1 data cache writes
				ICA = PAPI_L1_ICA, // L1 instruction cache accesses
				ICH = PAPI_L1_ICH, // L1 instruction cache hits
				ICM = PAPI_L1_ICM, // L1 instruction cache misses
				ICR = PAPI_L1_ICR, // L1 instruction cache reads
				ICW = PAPI_L1_ICW, // L1 instruction cache writes
				LDM = PAPI_L1_LDM, // L1 load misses
				STM = PAPI_L1_STM, // L1 store misses
				TCA = PAPI_L1_TCA, // L1 total cache accesses
				TCH = PAPI_L1_TCH, // L1 total cache hits
				TCM = PAPI_L1_TCM, // L1 total cache misses
				TCR = PAPI_L1_TCR, // L1 total cache reads
				TCW = PAPI_L1_TCW, // L1 total cache writes
			};

			enum class L2: type {
				DCA = PAPI_L2_DCA, // L2 data cache accesses
				DCH = PAPI_L2_DCH, // L2 data cache hits
				DCM = PAPI_L2_DCM, // L2 data cache misses
				DCR = PAPI_L2_DCR, // L2 data cache reads
				DCW = PAPI_L2_DCW, // L2 data cache writes
				ICA = PAPI_L2_ICA, // L2 instruction cache accesses
				ICH = PAPI_L2_ICH, // L2 instruction cache hits
				ICM = PAPI_L2_ICM, // L2 instruction cache misses
				ICR = PAPI_L2_ICR, // L2 instruction cache reads
				ICW = PAPI_L2_ICW, // L2 instruction cache writes
				LDM = PAPI_L2_LDM, // L2 load misses
				STM = PAPI_L2_STM, // L2 store misses
				TCA = PAPI_L2_TCA, // L2 total cache accesses
				TCH = PAPI_L2_TCH, // L2 total cache hits
				TCM = PAPI_L2_TCM, // L2 total cache misses
				TCR = PAPI_L2_TCR, // L2 total cache reads
				TCW = PAPI_L2_TCW, // L2 total cache writes
			};

			enum class L3: type {
				DCA = PAPI_L3_DCA, // L3 data cache accesses
				DCH = PAPI_L3_DCH, // L3 Data Cache Hits
				DCM = PAPI_L3_DCM, // L3 data cache misses
				DCR = PAPI_L3_DCR, // L3 data cache reads
				DCW = PAPI_L3_DCW, // L3 data cache writes
				ICA = PAPI_L3_ICA, // L3 instruction cache accesses
				ICH = PAPI_L3_ICH, // L3 instruction cache hits
				ICM = PAPI_L3_ICM, // L3 instruction cache misses
				ICR = PAPI_L3_ICR, // L3 instruction cache reads
				ICW = PAPI_L3_ICW, // L3 instruction cache writes
				LDM = PAPI_L3_LDM, // L3 load misses
				STM = PAPI_L3_STM, // L3 store misses
				TCA = PAPI_L3_TCA, // L3 total cache accesses
				TCH = PAPI_L3_TCH, // L3 total cache hits
				TCM = PAPI_L3_TCM, // L3 cache misses
				TCR = PAPI_L3_TCR, // L3 total cache reads
				TCW = PAPI_L3_TCW, // L3 total cache writes
			};
		}

		enum class TLB: type {
			DM = PAPI_TLB_DM, // Data translation lookaside buffer misses
			IM = PAPI_TLB_IM, // Instruction translation lookaside buffer misses
			SD = PAPI_TLB_SD, // Translation lookaside buffer shootdowns
			TL = PAPI_TLB_TL, // Total translation lookaside buffer misses
		};

		enum class DACCESS: type {
			LDI = PAPI_LD_INS,  // Load instructions
			LSI = PAPI_LST_INS, // Load/store instructions completed
			IDL = PAPI_LSU_IDL, // Cycles load/store units are idle
			RCY = PAPI_MEM_RCY, // Cycles Stalled Waiting for memory Reads
			SCY = PAPI_MEM_SCY, // Cycles Stalled Waiting for memory accesses
			WCY = PAPI_MEM_WCY, // Cycles Stalled Waiting for memory writes
			DM  = PAPI_PRF_DM,  // Data prefetch cache misses
			STL = PAPI_RES_STL, // Cycles stalled on any resource
			STI = PAPI_SR_INS,  // Store instructions
			CCY = PAPI_STL_CCY, // Cycles with no instructions completed
			ICY = PAPI_STL_ICY, // Cycles with no instruction issue
			SYI = PAPI_SYC_INS, // Synchronization instructions completed
		};
	}

	class PerfStat {
		public:
			PerfStat(/* std::initializer_list<int> cfg */) {
			}

		private:
	};
}
#endif
