#pragma once

#include <cstdint>
#include <iostream>

namespace prettyprint {

	class Format {
		public:
			virtual std::ostream & format(std::ostream & out) const = 0;
	};
	std::ostream & operator<<(std::ostream & out, const Format & f);

	class CSV: public Format {
		public:
			virtual std::ostream & format(std::ostream & out) const {
				return formatCSV(out);
			}

			const CSV & asCSV() const { return *this; }

		protected:
			template <typename T>
				std::ostream & sequence(std::ostream & out, const T & t) const {
					out << t << std::endl;
					return out;
				}

			template<typename T, typename... Args>
				std::ostream & sequence(std::ostream & out, const T & t, Args... args) const {
					out << t << ",";
					return sequence(out, args...);
				}

		private:
			virtual std::ostream & formatHeader(std::ostream & out) const = 0;
			virtual std::ostream & formatCSV(std::ostream & out) const = 0;

	};

	class Human: public Format {
		public:
			virtual std::ostream & format(std::ostream & out) const {
				return formatHuman(out);
			}

			const Human & asHuman() const { return *this; }

		private:
			virtual std::ostream & formatHuman(std::ostream & out) const = 0;
	};

	class Bytes: public Format {
		public:
			Bytes(uint64_t bytes);

			virtual std::ostream & format(std::ostream & out) const {
				return formatBytes(out);
			}

		private:
			uint64_t bytes;
			virtual std::ostream & formatBytes(std::ostream & out) const;
	};

}
