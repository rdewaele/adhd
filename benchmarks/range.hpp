#include "tuple-utils.hpp"

#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <tuple>

namespace adhd {
	// General interface for range types. Ranges have a lower and an upper bound,
	// and they support forward iteration from min to max. Implementors should
	// implement overflow semantics: if the current value equals max, and it is
	// incremented, the current value should reset to min.
	class RangeInterface {
		public:
			class iterator: public std::iterator<std::input_iterator_tag, RangeInterface> {
				public:
					typedef std::unique_ptr<RangeInterface> ptr_ri;

					iterator(const RangeInterface & ri): ptr(ri.clone()) {}
					iterator(ptr_ri & _ptr) { ptr.swap(_ptr); }
					iterator(const iterator & i): iterator(*i) {}

					iterator & operator++() { ptr->next(); return *this; }
					const RangeInterface & operator*() const { return **ptr; }

					bool operator==(const iterator & rhs) const { return ptr->equals(*(rhs.ptr)); }
					bool operator!=(const iterator & rhs) const { return !(ptr->equals(*(rhs.ptr))); }
					RangeInterface * operator->() { return ptr.get(); }

					friend inline std::ostream & operator<<(std::ostream & os, const iterator & up) {
						return up.ptr->toOStream(os);
					}

				private:
					ptr_ri ptr;
			};

			iterator begin() const {
				iterator && tmp(*this);
				tmp->gotoBegin();
				return tmp;
			}

			iterator end() const {
				iterator && tmp(*this);
				tmp->gotoEnd();
				return tmp;
			}

			virtual void gotoBegin() = 0;
			virtual void gotoEnd() = 0;
			virtual void next() = 0;
			virtual bool equals(const RangeInterface & rhs) const = 0;

			virtual RangeInterface * clone() const = 0;

			virtual bool atMin() const = 0;
			virtual bool atMax() const = 0;

			virtual std::ostream & toOStream(std::ostream & os) const = 0;

			RangeInterface & operator++() { next(); return *this; }
			const RangeInterface & operator*() const { return *this; }

			bool operator==(const RangeInterface & rhs) const { return equals(rhs); }
			bool operator!=(const RangeInterface & rhs) const { return !equals(rhs); }

			friend inline std::ostream & operator<<(std::ostream & os, const RangeInterface & ri) {
				return ri.toOStream(os);
			}

			virtual ~RangeInterface() = default;
	};

	// Range with overflow semantics: when current value would reach or surpass
	// max, it resets to the min value instead.
	// Two Range instances are considered equal when all fields are equal:
	// min, max, and current.
	template <typename T>
		class Range: public virtual RangeInterface {
			public:
				typedef T type;

				Range(T constant): Range(constant, constant) {}
				Range(T _min, T _max): Range(_min, _max, false) {
					if (min > max)
						throw std::logic_error("minimal value can not exceed maximal value");
				}

				virtual Range * clone() const override { return new Range(*this); }

				virtual void next() override {
					reset = (current >= max) || (++current > max);
					if (reset)
						current = min;
				}

				virtual bool equals(const RangeInterface & ri) const override {
					const Range & rhs = dynamic_cast<const Range &>(ri);
					return min == rhs.min
						&& max == rhs.max
						&& current == rhs.current
						&& reset == rhs.reset;
				}

				const Range & operator *() const { return *this; }

				T getValue() const { return current; }

				virtual bool atMin() const override { return min == current; }
				virtual bool atMax() const override { return max == current; }

				virtual void gotoBegin() override { current = min; reset = false; }
				virtual void gotoEnd() override { current = min; reset = true; }

				virtual std::ostream & toOStream (std::ostream & os) const override {
					return os << "[" << min << "," << max << "]" << ":" << current
						<< (reset ? " (reset)" : "");
				}

				const T min;
				const T max;

			private:
				Range(T _min, T _max, bool _reset)
					: min(_min), max(_max), current(_min), reset(_reset)
				{}

				T current;
				bool reset;
		};

	// Heterogeneous collection of Range types. Incrementing the set will
	// progressively increment its components from left to right, only proceeding
	// to a next component when the current one resets. This emulates the behaviour
	// of the iteration variables in a nested for loop, and will generate all
	// combinations of ranges in that order.
	template <typename ... TS>
		class RangeSet: public virtual RangeInterface {
			// XXX implementation could use some tlc to remove the dependency on the
			// tuple-utils header, as it doesn't fit the coding style of this project,
			// and only one or two templates are used from said header
			public:
				typedef RangeSet<TS ...> type;
				typedef std::tuple<Range<TS> ...> Fields;

				RangeSet(const Range<TS> & ... args): values(std::make_tuple(args ...)) {}
				RangeSet(const Fields & f): values(f) {}
				RangeSet(const RangeSet & c): values(c.values) {}

				virtual RangeSet * clone() const override { return new RangeSet(*this); }

				template <long unsigned int N>
					typename std::tuple_element<N, Fields>::type get() const {
						return std::get<N>(values);
					}

				template <long unsigned int N>
					typename std::tuple_element<N, Fields>::type::type getValue() const {
						return std::get<N>(values).getValue();
					}

				template <long unsigned int N>
					typename std::tuple_element<N, Fields>::type::type getMin() const {
						return std::get<N>(values).min;
					}

				template <long unsigned int N>
					typename std::tuple_element<N, Fields>::type::type getMax() const {
						return std::get<N>(values).max;
					}

				virtual void next() override { whileTrue(increment, values); }

				virtual bool equals(const RangeInterface & ri) const override {
					const RangeSet & rhs = dynamic_cast<const RangeSet &>(ri);
					return values == rhs.values;
				}

				const RangeSet & operator*() const { return *this; }

				template <typename RT, typename AT, RT (RangeInterface::*M)(), unsigned N>
					RT foo() {}

				template <typename RT, typename AT, RT (RangeInterface::*M)(), unsigned N, typename S, typename ... SS>
					RT foo() {
						(std::get<N>(values).*M)();
						foo<RT, AT, M, N+1, SS ...>();
					}

				template <typename RT, typename AT, RT (RangeInterface::*M)()>
					RT foo() { foo<RT, AT, M, 0, TS ...>(); }

				virtual void gotoBegin() override { foo<void, void, &RangeInterface::gotoBegin>(); }
				virtual void gotoEnd() override { foo<void, void, &RangeInterface::gotoEnd>(); }

				virtual bool atMin() const override { return whileTrue(atMinF, values); }
				virtual bool atMax() const override { return whileTrue(atMaxF, values); }

				template <typename ... TSS>
					RangeSet<TS ..., TSS ...> append(const RangeSet<TSS ...> & rs) {
						return std::tuple_cat(values, rs.values);
					}

				template <typename ... TSS>
					RangeSet<TS ..., TSS ...> prepend(const RangeSet<TSS ...> & rs) {
						return std::tuple_cat(values, rs.values);
					}

				virtual std::ostream & toOStream(std::ostream & os) const override {
					forEach(printer(os), values);
					return os;
				}

			private:
				Fields values;

				struct printer {
					printer(std::ostream & os): out(os) {}
					template <typename T>
						std::ostream & operator() (const T & t) const {
							return out << t << " ";
						}
					std::ostream & out;
				};

				struct {
					template <typename T>
						bool operator() (T & t) const {
							return (++t).atMin();
						}
				} increment {};

				struct {
					template <typename T>
						bool operator() (T & t) const {
							return t.atMin();
						}
				} atMinF {};

				struct {
					template <typename T>
						bool operator() (T & t) const {
							return t.atMax();
						}
				} atMaxF {};
		};
}
