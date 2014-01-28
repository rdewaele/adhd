#pragma once

#if 1
namespace adhd {
	class Config {
	};
}
#else
namespace adhd {

	// Config represents a configuration for a benchmark. A configuration may
	// conceptually define a loop, e.g. "this benchmark should be run for arrays
	// of different sizes". Generalizing this idea, a configuration may iterate
	// through multiple variables like these. To accommodate for this, an
	// iterator interface and the State class are defined. A state of a
	// configuration represents an instance where all configuration parameters
	// have a single value. In other words, every so called state of a
	// configuration defines a specific benchmark that should be run.

	// This mechanic effectively ties the implicit loops defined by
	// configurations to the object representing them, thereby allowing clients
	// to iterate through all configuration states in an abstract manner.

	namespace Configs {

		// range with overflow semantics: when max value is increased,
		// it resets to the min value
		// ranges are considered equal when all fields are equal
		template <typename T>
			class Range {
				public:
					Range(T _min, T _max): min(_min), max(_max), current(_min) {}

					Range<T> & operator++() {
						if (current < max)
							++current;
						else
							current = min;
						return *this;
					}

					Range operator++(int) {
						Range<T> && retval = Range<T>(*this);
						operator++();
						return retval;
					}

					bool operator=(const Range<T> & rhs) {
						return min == rhs.min && max == rhs.max && current == rhs.current;
					}

				private:
					const T min;
					const T max;
					T current;
			};

		template <typename... TS>
			class Config {
				public:
					template <typename ENUM>
					typename std::tuple_element<TS...>::type<> get(ENUM name) {
						return std::get<name>(values);
					}
				private:
					std::tuple<TS...> values;
			};

		enum class TestFields { first, second };
		class Test: public Config<TestFields, bool, int> {
			bool getFirst() {
				return get<int>(TestFields::first);
			}
		};
	}

	class Config {
		public:
			// config state interface
			class State;
			// iterator interface for different config states
			class Iterator;

			class StateFactory {
				public:
					virtual State * begin(const Config & cfg) const = 0;
					virtual State * end(const Config & cfg) const = 0;
					virtual StateFactory * clone() const = 0;
					virtual ~StateFactory() = default;
			};

			class State {
				//TODO figure out a better way to define begin- and end states
				// e.g. by passing a reference of the class so that the state itself
				// can define where it begins and ends
				public:
					virtual bool equals(const State &) const = 0;
					virtual State * clone() const = 0;
					State() = default;
					virtual ~State() = default;
				protected:
					State(const State & s);
					//TODO virtual bool atBegin() = 0;
					virtual bool atEnd() = 0;
					//TODO virtual void reset() = 0;
				private:
					virtual void next() = 0;
					friend class Config::Iterator;
			};

			class Iterator: public std::iterator<std::input_iterator_tag, State> {
				public:
					Iterator(State * _state): state(_state) {}

					Iterator & operator++() {
						state->next();
						return *this;
					}

					Iterator operator++(int) {
						Iterator tmp(state->clone());
						operator++();
						return tmp;
					}

					bool operator==(const Iterator & rhs) {
						return state->equals(*rhs.state);
					}

					bool operator!=(const Iterator & rhs) {
						return !(*this == rhs);
					}

					const std::shared_ptr<State> operator*() {
						State * tmp = state->clone();
						return std::shared_ptr<State>(tmp);
					}

				private:
					std::unique_ptr<State> state;
			};

			virtual ~Config() = default;

			virtual Config * clone() const = 0;

			Iterator begin() const {
				return Iterator(statefactory->begin(*this));
			}

			Iterator end() const {
				return Iterator(statefactory->end(*this));
			}

		private:
//			static_assert(std::is_base_of<SF, StateFactory>::value,
//					"Config state factory template argument must inherit from StateFactory.");
			std::unique_ptr<StateFactory> statefactory;
	};

	class EmptyConfig: public Config {
		public:
			// SingleRun defines only two states: the begin state, which is empty, and
			// the end state, which serves as and end sentinel value.
			class SingleRun: public State {
				public:
					SingleRun(bool _end = false): end(_end) {}

					virtual bool equals(const State & s) const final override {
						const SingleRun & sr = dynamic_cast<const SingleRun &>(s);
						return sr.end == end;
					}

					virtual void next() final override {
						end = true;
					}

					virtual bool atEnd() final override {
						return end;
					}

					virtual SingleRun * clone() const final override {
						return new SingleRun(end);
					}

				private:
					bool end;
			};

			virtual EmptyConfig * clone() const {
				return new EmptyConfig();
			}

			virtual ~EmptyConfig() = default;
	};

	// The ThreadedConfig class defines a min/max range for the number of
	// threads a benchmark should be run with. Iterating through the config will
	// increment the 'current' number of threads by one, starting at min, and
	// ending at max.
	class ThreadedConfig: public Config {
		public:
			ThreadedConfig(unsigned min, unsigned max):
				minThreads(min), maxThreads(max)
			{
				if (0 == min)
					throw std::runtime_error("minimal amount of threads must be > 0");
				if (max < min)
					throw std::runtime_error("maximal amount of threads must be >= minimal amount of threads");
			}

			// ThreadedState iterates from min to max.
			class ThreadedState: public State {
				public:
					ThreadedState(const unsigned min, const unsigned _max):
						current(min),
						max(_max)
				{}

					virtual bool equals(const State & s) const override {
						const ThreadedState & ts = dynamic_cast<const ThreadedState &>(s);
						return ts.current == current && ts.max == max;
					}

					virtual void next() override {
						if (!atEnd())
							++current;
					}

					virtual bool atEnd() override {
						return !(current < max);
					}

					virtual ThreadedState * clone() const override {
						return new ThreadedState(current, max);
					}

					unsigned getCurrent() const { return current; }
					unsigned getMax() const { return max; }

				private:
					unsigned current;
					const unsigned max;
			};

			virtual ThreadedConfig * clone() const override {
				return new ThreadedConfig(minThreads, maxThreads);
			}

			const unsigned minThreads;
			const unsigned maxThreads;
	};
}
#endif
