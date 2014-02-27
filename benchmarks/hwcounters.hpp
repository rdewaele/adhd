#pragma once

#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

#include <papi.h>

namespace adhd {
	namespace hwcounters {
		using enum_t = int;

		namespace names {
			constexpr const struct entry {
				const enum_t key;
				const char * const val;
			}  mapping[] {
				// Conditional Branching
				{ PAPI_BR_CN, "Conditional branch instructions" },
				{ PAPI_BR_INS, "Branch instructions" },
				{ PAPI_BR_MSP, "Conditional branch instructions mispredicted" },
				{ PAPI_BR_NTK, "Conditional branch instructions not taken" },
				{ PAPI_BR_PRC, "Conditional branch instructions correctly predicted" },
				{ PAPI_BR_TKN, "Conditional branch instructions taken" },
				{ PAPI_BR_UCN, "Unconditional branch instructions" },
				{ PAPI_BRU_IDL, "Cycles branch units are idle" },
				{ PAPI_BTAC_M, "Branch target address cache misses" },

				// Cache Requests:
				{ PAPI_CA_CLN, "Requests for exclusive access to clean cache line" },
				{ PAPI_CA_INV, "Requests for cache line invalidation" },
				{ PAPI_CA_ITV, "Requests for cache line intervention" },
				{ PAPI_CA_SHR, "Requests for exclusive access to shared cache line" },
				{ PAPI_CA_SNP, "Requests for a snoop" },

				// Conditional Store:
				{ PAPI_CSR_FAL, "Failed store conditional instructions" },
				{ PAPI_CSR_SUC, "Successful store conditional instructions" },
				{ PAPI_CSR_TOT, "Total store conditional instructions" },

				// Floating Point Operations:
				{ PAPI_FAD_INS, "Floating point add instructions" },
				{ PAPI_FDV_INS, "Floating point divide instructions" },
				{ PAPI_FMA_INS, "FMA instructions completed" },
				{ PAPI_FML_INS, "Floating point multiply instructions" },
				{ PAPI_FNV_INS, "Floating point inverse instructions" },
				{ PAPI_FP_INS, "Floating point instructions" },
				{ PAPI_FP_OPS, "Floating point operations" },
				{ PAPI_FP_STAL, "Cycles the FP unit" },
				{ PAPI_FPU_IDL, "Cycles floating point units are idle" },
				{ PAPI_FSQ_INS, "Floating point square root instructions" },
				{ PAPI_SP_OPS, "Floating point operations executed; optimized to count scaled single precision vector operations" },
				{ PAPI_DP_OPS, "Floating point operations executed; optimized to count scaled double precision vector operations" },
				{ PAPI_VEC_SP, "Single precision vector/SIMD instructions" },
				{ PAPI_VEC_DP, "Double precision vector/SIMD instructions" },

				// Instruction Counting:
				{ PAPI_FUL_CCY, "Cycles with maximum instructions completed" },
				{ PAPI_FUL_ICY, "Cycles with maximum instruction issue" },
				{ PAPI_FXU_IDL, "Cycles integer units are idle" },
				{ PAPI_HW_INT, "Hardware interrupts" },
				{ PAPI_INT_INS, "Integer instructions" },
				{ PAPI_TOT_CYC, "Total cycles" },
				{ PAPI_TOT_IIS, "Instructions issued" },
				{ PAPI_TOT_INS, "Instructions completed" },
				{ PAPI_VEC_INS, "Vector/SIMD instructions" },

				// Cache Access:
				{ PAPI_L1_DCA, "L1 data cache accesses" },
				{ PAPI_L1_DCH, "L1 data cache hits" },
				{ PAPI_L1_DCM, "L1 data cache misses" },
				{ PAPI_L1_DCR, "L1 data cache reads" },
				{ PAPI_L1_DCW, "L1 data cache writes" },
				{ PAPI_L1_ICA, "L1 instruction cache accesses" },
				{ PAPI_L1_ICH, "L1 instruction cache hits" },
				{ PAPI_L1_ICM, "L1 instruction cache misses" },
				{ PAPI_L1_ICR, "L1 instruction cache reads" },
				{ PAPI_L1_ICW, "L1 instruction cache writes" },
				{ PAPI_L1_LDM, "L1 load misses" },
				{ PAPI_L1_STM, "L1 store misses" },
				{ PAPI_L1_TCA, "L1 total cache accesses" },
				{ PAPI_L1_TCH, "L1 total cache hits" },
				{ PAPI_L1_TCM, "L1 total cache misses" },
				{ PAPI_L1_TCR, "L1 total cache reads" },
				{ PAPI_L1_TCW, "L1 total cache writes" },
				{ PAPI_L2_DCA, "L2 data cache accesses" },
				{ PAPI_L2_DCH, "L2 data cache hits" },
				{ PAPI_L2_DCM, "L2 data cache misses" },
				{ PAPI_L2_DCR, "L2 data cache reads" },
				{ PAPI_L2_DCW, "L2 data cache writes" },
				{ PAPI_L2_ICA, "L2 instruction cache accesses" },
				{ PAPI_L2_ICH, "L2 instruction cache hits" },
				{ PAPI_L2_ICM, "L2 instruction cache misses" },
				{ PAPI_L2_ICR, "L2 instruction cache reads" },
				{ PAPI_L2_ICW, "L2 instruction cache writes" },
				{ PAPI_L2_LDM, "L2 load misses" },
				{ PAPI_L2_STM, "L2 store misses" },
				{ PAPI_L2_TCA, "L2 total cache accesses" },
				{ PAPI_L2_TCH, "L2 total cache hits" },
				{ PAPI_L2_TCM, "L2 total cache misses" },
				{ PAPI_L2_TCR, "L2 total cache reads" },
				{ PAPI_L2_TCW, "L2 total cache writes" },
				{ PAPI_L3_DCA, "L3 data cache accesses" },
				{ PAPI_L3_DCH, "L3 Data Cache Hits" },
				{ PAPI_L3_DCM, "L3 data cache misses" },
				{ PAPI_L3_DCR, "L3 data cache reads" },
				{ PAPI_L3_DCW, "L3 data cache writes" },
				{ PAPI_L3_ICA, "L3 instruction cache accesses" },
				{ PAPI_L3_ICH, "L3 instruction cache hits" },
				{ PAPI_L3_ICM, "L3 instruction cache misses" },
				{ PAPI_L3_ICR, "L3 instruction cache reads" },
				{ PAPI_L3_ICW, "L3 instruction cache writes" },
				{ PAPI_L3_LDM, "L3 load misses" },
				{ PAPI_L3_STM, "L3 store misses" },
				{ PAPI_L3_TCA, "L3 total cache accesses" },
				{ PAPI_L3_TCH, "L3 total cache hits" },
				{ PAPI_L3_TCM, "L3 cache misses" },
				{ PAPI_L3_TCR, "L3 total cache reads" },
				{ PAPI_L3_TCW, "L3 total cache writes" },

				// Data Access:
				{ PAPI_LD_INS, "Load instructions" },
				{ PAPI_LST_INS, "Load/store instructions completed" },
				{ PAPI_LSU_IDL, "Cycles load/store units are idle" },
				{ PAPI_MEM_RCY, "Cycles Stalled Waiting for memory Reads" },
				{ PAPI_MEM_SCY, "Cycles Stalled Waiting for memory accesses" },
				{ PAPI_MEM_WCY, "Cycles Stalled Waiting for memory writes" },
				{ PAPI_PRF_DM, "Data prefetch cache misses" },
				{ PAPI_RES_STL, "Cycles stalled on any resource" },
				{ PAPI_SR_INS, "Store instructions" },
				{ PAPI_STL_CCY, "Cycles with no instructions completed" },
				{ PAPI_STL_ICY, "Cycles with no instruction issue" },
				{ PAPI_SYC_INS, "Synchronization instructions completed" },

				// TLB Operations:
				{ PAPI_TLB_DM, "Data translation lookaside buffer misses" },
				{ PAPI_TLB_IM, "Instruction translation lookaside buffer misses" },
				{ PAPI_TLB_SD, "Translation lookaside buffer shootdowns" },
				{ PAPI_TLB_TL, "Total translation lookaside buffer misses" },
			};

			constexpr static const char * search(const enum_t key, const size_t idx) {
				return idx >= sizeof(mapping) / sizeof(decltype(*mapping)) ? "NOTFOUND" :
					(key == mapping[idx].key ? mapping[idx].val : search(key, idx + 1));
			}

			constexpr static const char * lookup(enum_t key) {
				return search(key, 0);
			}
		}

		enum class branching: enum_t {
			CNI = PAPI_BR_CN,   // Conditional branch instructions
			INS = PAPI_BR_INS,  // Branch instructions
			MSP = PAPI_BR_MSP,  // Conditional branch instructions mispredicted
			NTK = PAPI_BR_NTK,  // Conditional branch instructions not taken
			PRC = PAPI_BR_PRC,  // Conditional branch instructions correctly predicted
			TKN = PAPI_BR_TKN,  // Conditional branch instructions taken
			UCN = PAPI_BR_UCN,  // Unconditional branch instructions
			IDL = PAPI_BRU_IDL, // Cycles branch units are idle
			BTM = PAPI_BTAC_M,  // Branch target address cache misses
		};

		enum class stores: enum_t {
			FAL = PAPI_CSR_FAL, // Failed store conditional instructions
			SUC = PAPI_CSR_SUC, // Successful store conditional instructions
			TOT = PAPI_CSR_TOT, // Total store conditional instructions
		};

		namespace floating {
			enum class instructions: enum_t {
				FAD = PAPI_FAD_INS, // Floating point add instructions
				FDV = PAPI_FDV_INS, // Floating point divide instructions
				FMA = PAPI_FMA_INS, // FMA instructions completed
				FML = PAPI_FML_INS, // Floating point multiply instructions
				FNV = PAPI_FNV_INS, // Floating point inverse instructions
				FP  = PAPI_FP_INS,  // Floating point instructions
				FSQ = PAPI_FSQ_INS, // Floating point square root instructions
				VSP = PAPI_VEC_SP, // Single precision vector/SIMD instructions
				VDP = PAPI_VEC_DP, // Double precision vector/SIMD instructions
			};

			enum class operations: enum_t {
				FP = PAPI_FP_OPS, // Floating point operations
				SP = PAPI_SP_OPS, // Floating point operations executed; optimized to count scaled single precision vector operations
				DP = PAPI_DP_OPS, // Floating point operations executed; optimized to count scaled double precision vector operations
			};

			enum class efficiency: enum_t {
				STL = PAPI_FP_STAL, // Cycles the FP unit
				IDL = PAPI_FPU_IDL, // Cycles floating point units are idle
			};
		}

		namespace cache {
			enum class requests: enum_t {
				CLN = PAPI_CA_CLN, // Requests for exclusive access to clean cache line
				INV = PAPI_CA_INV, // Requests for cache line invalidation
				ITV = PAPI_CA_ITV, // Requests for cache line intervention
				SHR = PAPI_CA_SHR, // Requests for exclusive access to shared cache line
				SNP = PAPI_CA_SNP, // Requests for a snoop
			};

			enum class L1: enum_t {
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

			enum class L2: enum_t {
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

			enum class L3: enum_t {
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

		enum class TLB: enum_t {
			DM = PAPI_TLB_DM, // Data translation lookaside buffer misses
			IM = PAPI_TLB_IM, // Instruction translation lookaside buffer misses
			SD = PAPI_TLB_SD, // Translation lookaside buffer shootdowns
			TL = PAPI_TLB_TL, // Total translation lookaside buffer misses
		};

		enum class data_access: enum_t {
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

	// PAPI lists have an effective size type of int (e.g. array of events, values, ...)
	using PAPI_size_t = int;

	class Events: private std::vector<hwcounters::enum_t> {
		public:
			class event_t {
				private:
					using enum_t = hwcounters::enum_t;
					hwcounters::enum_t val;
					friend class Events;

				public:
					event_t(const hwcounters::branching & value)
						: val(static_cast<enum_t>(value)) {}
					event_t(const hwcounters::stores & value)
						: val(static_cast<enum_t>(value)) {}
					event_t(const hwcounters::floating::instructions & value)
						: val(static_cast<enum_t>(value)) {}
					event_t(const hwcounters::floating::operations & value)
						: val(static_cast<enum_t>(value)) {}
					event_t(const hwcounters::floating::efficiency & value)
						: val(static_cast<enum_t>(value)) {}
					event_t(const hwcounters::cache::requests & value)
						: val(static_cast<enum_t>(value)) {}
					event_t(const hwcounters::cache::L1 & value)
						: val(static_cast<enum_t>(value)) {}
					event_t(const hwcounters::cache::L2 & value)
						: val(static_cast<enum_t>(value)) {}
					event_t(const hwcounters::cache::L3 & value)
						: val(static_cast<enum_t>(value)) {}
					event_t(const hwcounters::TLB & value)
						: val(static_cast<enum_t>(value)) {}
					event_t(const hwcounters::data_access & value)
						: val(static_cast<enum_t>(value)) {}
			};

			template <typename T1, typename T2>
				inline typename std::enable_if<(std::is_signed<T1>::value == std::is_signed<T2>::value), bool>::type
				is_oob(const T1 & val) {
					using T2_limits = std::numeric_limits<T2>;
					return val > T2_limits::max() || val < T2_limits::lowest();
				}

			template <typename T1, typename T2>
				inline typename std::enable_if<(std::is_signed<T1>::value != std::is_signed<T2>::value), bool>::type
				is_oob(const T1 & val) {
					return val < 0 || val > std::numeric_limits<T2>::max();
				}

			// event_t init list
			using init_event_t = std::initializer_list<event_t>;

			// construct a list of events, making sure all event_t values are valid
			Events(init_event_t init) {
				if (is_oob<init_event_t::size_type, PAPI_size_t>(init.size()))
					throw std::range_error("Events: number of events too large");
				for (const auto & val: init)
					push_back(val.val);
			}

			// facilitate PAPI interaction
			inline PAPI_size_t size() const {
				return static_cast<PAPI_size_t>(std::vector<hwcounters::enum_t>::size());
			}

			// slice the valid enum guarantees, but receive a more generic vector
			std::vector<hwcounters::enum_t> to_vector() const {
				return *this;
			}
	};

	class PerfStat {
		public:
			using value_t = long_long;

			PerfStat(const Events & ev)
				: num_events(ev.size()), events(ev.to_vector()), values(num_events),
				events_data(events.data()), values_data(values.data())
			{
				const PAPI_size_t num_ctrs = PAPI_num_counters();
				papi_error_check(num_ctrs, num_ctrs <= PAPI_OK);

				if (num_ctrs < num_events) {
					std::stringstream err;
					err << "PerfStat: not enough counters available; requested "
						<< events.size() << ", have " << num_ctrs;
					throw std::runtime_error(err.str());
				}
			}

			inline void start() {
				const int rv = PAPI_start_counters(events.data(), num_events);
				papi_error_check(rv, rv != PAPI_OK);
			}

			// TODO: inline void read() {}

			const inline PerfStat & stop() {
				const int rv = PAPI_stop_counters(values.data(), num_events);
				papi_error_check(rv, rv != PAPI_OK);
				return *this;
			}

			const inline std::vector<value_t> & getValues() const {
				return values;
			}

			friend inline
				std::ostream & operator<<(std::ostream & os, const PerfStat & ps) {
					for (PAPI_size_t i = 0; i < ps.num_events; ++i)
						os << hwcounters::names::lookup(ps.events[i])
							<< ": " << ps.values[i] << std::endl;
					return os;
				}

		private:
			PAPI_size_t num_events;
			std::vector<hwcounters::enum_t> events;
			std::vector<value_t> values;
			hwcounters::enum_t * const events_data;
			value_t * const values_data;

			static inline void papi_error_check (int errval, bool errcond = true) {
				if (errcond) {
					std::stringstream err;
					err << "PerfStat - PAPI error: " << PAPI_strerror(errval);
					throw std::runtime_error(err.str());
				}
			}
	};
}
