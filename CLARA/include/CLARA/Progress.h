#pragma once
#include <CLARA/Common.h>

namespace CLARA::CLASM {

	class IReporter {
	public:
		virtual ~IReporter() = default;

		virtual auto info(string_view) const->void = 0;
		virtual auto warn(string_view) const->void = 0;
		virtual auto error(string_view) const->void = 0;
	};

}