#pragma once

#include <iostream>
#include <iterator>
#include <memory>

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

	// The basic Config class represents an empty configuration, and has only a
	// single (empty) state.

	class Config {
		public:
			virtual Config * clone() const {
				return new Config();
			}

			class State {
				public:
					virtual bool equals(const State &) const = 0;
					virtual void next() = 0;
					virtual State * clone() const = 0;
					virtual ~State() = default;
			};

			// SingleRun defines only two states: the begin state, which is empty, and
			// the end state, which serves as and end sentinel value.
			class SingleRun: public State {
				public:
					SingleRun(bool _end = false):
						end(_end)
				{}

					virtual bool equals(const State & s) const final override {
						const SingleRun & sr = dynamic_cast<const SingleRun &>(s);
						return sr.end == end;
					}

					virtual void next() final override {
						end = true;
					}

					virtual SingleRun * clone() const final override {
						return new SingleRun(*this);
					}
				private:

					bool end;
			};

			class Iterator;

			template <typename T>
				T get(const std::string & s) const;

			virtual Iterator begin() const {
				return Iterator(SingleRun(false));
			}

			virtual Iterator end() const {
				return Iterator(SingleRun(true));
			}

			virtual ~Config() = default;

			// iterator interface for different config states
			class Iterator: public std::iterator<std::input_iterator_tag, State> {
				public:
					Iterator(const State & _state):
						state(_state.clone())
				{}

					Iterator & operator++() {
						state->next();
						return *this;
					}

					Iterator operator++(int) {
						Iterator tmp(*state);
						operator++();
						return tmp;
					}

					bool operator==(const Iterator & rhs) {
						return state->equals(*rhs.state);
					}

					bool operator!=(const Iterator & rhs) {
						return !(*this == rhs);
					}

					const State & operator*() {
						return *state;
					}

				private:
					std::shared_ptr<State> state;
			};
	};
}
