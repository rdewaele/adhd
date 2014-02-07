#pragma once

#include "prettyprint.hpp"

#include <functional>
#include <iostream>

namespace adhd {

	class Timings: public prettyprint::CSV, public prettyprint::Human {
		public:
			virtual ~Timings() = default;
			virtual std::ostream & formatHeader(std::ostream & out) const override = 0;
			virtual std::ostream & formatCSV(std::ostream & out) const override = 0;
			virtual std::ostream & formatHuman(std::ostream & out) const override = 0;
	};

	typedef void timing_cb_t(const Timings &);
	typedef std::function<timing_cb_t> timing_cb;

}
