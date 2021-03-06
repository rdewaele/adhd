#pragma once

#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>

namespace adhd {
	// General interface for range types. Ranges have a lower and an upper bound,
	// and they support forward iteration from min to max. Implementors should
	// implement overflow semantics: if the current value equals max, and it is
	// incremented, the current value should reset to min.
	class RangeInterface {
		public:
			template <typename W>
				class iterator: public std::iterator<std::input_iterator_tag, W> {
					private:
						using ri_ptr = std::unique_ptr<W>;

					public:
						iterator(const W & ri)
							: ptr(ri.clone())
						{}

						iterator(ri_ptr & _ptr) {
							ptr.swap(_ptr);
						}

						iterator(const iterator & i)
							: iterator(*(i.ptr))
						{}

						iterator & operator++() {
							ptr->next(); return *this;
						}

						W & operator*() {
							return *(ptr.get());
						}

						bool operator==(const iterator & rhs) const {
							return *ptr == *(rhs.ptr);
						}

						bool operator!=(const iterator & rhs) const {
							return *ptr != *(rhs.ptr);
						}

						W * operator->() {
							return ptr.get();
						}

						friend inline
							std::ostream & operator<<(std::ostream & os, const iterator & rii) {
								return os << *rii;
							}

					private:
						ri_ptr ptr;
				};

			virtual void gotoBegin() = 0;
			virtual void gotoEnd() = 0;
			virtual void next() = 0;

			virtual RangeInterface * clone() const = 0;

			virtual bool atMin() const = 0;
			virtual bool atMax() const = 0;

			RangeInterface & operator++() {
				next();
				return *this;
			}

			// XXX '= default' seems nicer, but icpc rejects this with error #809
			// when the destructor is overridden
			virtual ~RangeInterface() {};
	};

	template <typename W>
		RangeInterface::iterator<W> begin(const W & p) {
			RangeInterface::iterator<W> tmp(p);
			tmp->gotoBegin();
			return tmp;
		}

	template <typename W>
		RangeInterface::iterator<W> end(const W & p) {
			RangeInterface::iterator<W> tmp(p);
			tmp->gotoEnd();
			return tmp;
		}

	template <typename W>
		RangeInterface::iterator<W> begin(const W * p) {
			return begin(*p);
		}

	template <typename W>
		RangeInterface::iterator<W> end(const W * p) {
			return end(*p);
		}

	// Range with overflow semantics: when current value would reach or surpass
	// max, it resets to the min value instead.
	// Two Range instances are considered equal when all fields are equal:
	// min, max, and current.
	template <typename T>
		class AffineStepper: public virtual RangeInterface {
			public:
				AffineStepper(T constant): AffineStepper(constant, constant) {}
				AffineStepper(T min, T max, T mul = 1, T inc = 1)
					: AffineStepper(min < max ? min : max, min < max ? max : min, mul, inc, false)
					{}

				inline T increment(T & value) {
					return value = mulValue * value + incValue;
				}

				// TODO maybe warn when the exact maxValue is never reached: this could
				// be unintentional behaviour from the client's side
				virtual void next() override {
					reset = (current >= maxValue) || (increment(current) > maxValue);
					if (reset)
						current = minValue;
				}

				virtual AffineStepper * clone() const override {
					return new AffineStepper(*this);
				}

				bool operator==(const AffineStepper & rhs) const {
					return minValue == rhs.minValue
						&& maxValue == rhs.maxValue
						&& mulValue == rhs.mulValue
						&& incValue == rhs.incValue
						&& current == rhs.current
						&& reset == rhs.reset;
				}

				bool operator!=(const AffineStepper & rhs) const {
					return !operator==(rhs);
				}

				T getValue() const {
					return current;
				}

				virtual bool atMin() const override {
					return minValue == current;
				}

				virtual bool atMax() const override {
					return maxValue == current;
				}

				virtual void gotoBegin() override {
					current = minValue;
					reset = false;
				}

				virtual void gotoEnd() override {
					current = minValue;
					reset = true;
				}

				friend inline
					std::ostream & operator<<(std::ostream & os, const AffineStepper & r) {
						return os << "[" << r.minValue << "," << r.maxValue << "]:"
							<< r.current << (r.reset ? " (reset)" : "");
					}

				const T minValue;
				const T maxValue;
				const T mulValue;
				const T incValue;

			private:
				AffineStepper(T min, T max, T mul, T inc, bool _reset)
					: minValue(min), maxValue(max), mulValue(mul), incValue(inc),
					current(min), reset(_reset)
			{}

				T current;
				bool reset;
		};

	// TODO: further extend the explicit stepper to work with any container type
	// supporting iterators
	// TODO the second: maybe move the minValue maxValue getValue getters to
	// the rangeinterface
	template <typename T>
		class ExplicitStepper: public virtual RangeInterface {
			private:
				using values_t = std::vector<T>;
				using values_iterator = typename values_t::const_iterator;
				using values_ptr = std::shared_ptr<values_t>;

			public:
				ExplicitStepper(std::initializer_list<T> vals)
					: reset(false), values(new values_t(vals)),
					current(values->cbegin()),
					minValue(*current), maxValue(*--values->cend())
			{}

				virtual void next() override {
					reset = (++current) == values->cend();
					if (reset)
						current = values->cbegin();
				}

				virtual ExplicitStepper * clone() const override {
					return new ExplicitStepper(*this);
				}

				bool operator==(const ExplicitStepper & rhs) {
					return values == rhs.values
						&& current == rhs.current
						&& reset == rhs.reset;
				}

				bool operator!=(const ExplicitStepper & rhs) {
					return !operator==(rhs);
				}

				inline T getValue() const {
					return *current;
				}

				virtual bool atMin() const override {
					return values->cbegin() == current;
				}

				virtual bool atMax() const override {
					return values->cend() == current;
				}

				virtual void gotoBegin() override {
					current = values->cbegin();
					reset = false;
				}

				virtual void gotoEnd() override {
					current = values->cbegin();
					reset = true;
				}

				friend inline
					std::ostream & operator<<(std::ostream & os, const ExplicitStepper & es) {
						return os << "<" << es.minValue << "..." << es.maxValue << ">:"
							<< es.getValue() << (es.reset ? " (reset)" : "");
					}

			private:
				bool reset;
				const values_ptr values;
				values_iterator current;

			public:
				const T minValue;
				const T maxValue;
		};

	// Heterogeneous collection of Range types. Incrementing the set will
	// progressively increment its components from left to right, only proceeding
	// to a next component when the current one resets. This emulates the behaviour
	// of the iteration variables in a nested for loop, and will generate all
	// combinations of ranges in that order.
	template <typename ... TS>
		class RangeSet: public virtual RangeInterface {
			private:
				using Fields = std::tuple<TS ...>;

				template <long unsigned int N>
					using field_t = typename std::tuple_element<N, Fields>::type;

			public:
				RangeSet(const TS & ... args): values(std::make_tuple(args ...)) {}
				RangeSet(const Fields & f): values(f) {}
				RangeSet(const RangeSet & c): values(c.values) {}

				virtual RangeSet * clone() const override {
					return new RangeSet(*this);
				}

				template <long unsigned int N>
					const field_t<N> & get() const {
						return std::get<N>(values);
					}

				template <long unsigned int N>
					auto getValue() const -> decltype(this->get<N>().getValue()) {
						return get<N>().getValue();
					}

				template <long unsigned int N>
					decltype(field_t<N>::minValue) getMinValue() const {
						return std::get<N>(values).minValue;
					}

				template <long unsigned int N>
					decltype(field_t<N>::maxValue) getMaxValue() const {
						return std::get<N>(values).maxValue;
					}

				virtual void next() override {
					whileTrue(&field_next);
				}

				bool operator==(const RangeSet & rhs) const {
					return values == rhs.values;
				}

				bool operator!=(const RangeSet & rhs) const {
					return values != rhs.values;
				}

				virtual void gotoBegin() override {
					whileTrue(&field_gotoBegin);
				}

				virtual void gotoEnd() override {
					whileTrue(&field_gotoEnd);
				}

				virtual bool atMin() const override {
					return whileTrue(&field_atMin);
				}

				virtual bool atMax() const override {
					return whileTrue(&field_atMax);
				}

				template <typename ... TSS>
					RangeSet<TS ..., TSS ...> append(const RangeSet<TSS ...> & rs) {
						return std::tuple_cat(values, rs.values);
					}

				template <typename ... TSS>
					RangeSet<TS ..., TSS ...> prepend(const RangeSet<TSS ...> & rs) {
						return std::tuple_cat(values, rs.values);
					}

				friend inline
					std::ostream & operator<<(std::ostream & os, const RangeSet & rs) {
						return rs.printfields(os);
					}

			private:
				Fields values;

				// helper functions

				// whiletrue applies a function to each separate field
				// as long as the function application evaluates to true
				template <std::size_t I = 0, typename FuncT, typename ... FuncArgs>
					inline typename std::enable_if<I == sizeof...(TS), bool>::type
					whileTrue(FuncT, FuncArgs & ...) {
						return true;
					}

				template <std::size_t I = 0, typename FuncT, typename ... FuncArgs>
					inline typename std::enable_if<I < sizeof...(TS), bool>::type
					whileTrue(FuncT f, FuncArgs & ... fa) {
						return f(std::get<I>(values), fa ...)
							&& whileTrue<I +  1, FuncT>(f, fa ...);
					}

				// whiletrue const-qualified variant
				template <std::size_t I = 0, typename FuncT, typename ... FuncArgs>
					inline typename std::enable_if<I == sizeof...(TS), bool>::type
					whileTrue(FuncT, FuncArgs & ...) const {
						return true;
					}

				template <std::size_t I = 0, typename FuncT, typename ... FuncArgs>
					inline typename std::enable_if<I < sizeof...(TS), bool>::type
					whileTrue(FuncT f, FuncArgs & ... fa) const {
						return f(std::get<I>(values), fa ...)
							&& whileTrue<I +  1, FuncT>(f, fa ...);
					}


				// printfields applies operator<< to every field contained in this RangeSet
				// this enables proper overloading on operator<< for the contained fields
				// (cfr. alternative using whileTrue, which would upcast all fields to
				//  RangeInterface, which in turn would require an overridable method in
				//  the interface to indirectly apply the operator<<.)
				template <std::size_t I = 0>
					typename std::enable_if<I + 1 == sizeof...(TS), std::ostream &>::type
					printfields(std::ostream & os) const {
						return os << std::get<I>(values);
					}

				template <std::size_t I = 0>
					typename std::enable_if<I + 1 < sizeof...(TS), std::ostream &>::type
					printfields(std::ostream & os) const {
						os << std::get<I>(values) << " ";
						return printfields<I + 1>(os);
					}


				// wrappers conforming to whiletrue interface

				static inline bool field_next(RangeInterface & r) {
					r.next();
					return r.atMin();
				}

				static inline bool field_atMin(const RangeInterface & r) {
					return r.atMin();
				}

				static inline bool field_atMax(const RangeInterface & r) {
					return r.atMax();
				}

				static inline bool field_gotoBegin(RangeInterface & r) {
					r.gotoBegin();
					return true;
				}

				static inline bool field_gotoEnd(RangeInterface & r) {
					r.gotoEnd();
					return true;
				}
		};
}
