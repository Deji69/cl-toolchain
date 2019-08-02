#pragma once
#include <CLASM/Common/Macros.h>
#include <CLASM/Common/Imports.h>
#include <CLASM/Common/Algorithm.h>
#include <CLASM/Common/String.h>
#include <CLASM/Common/Literals.h>

namespace CLARA {
	namespace CLASM {
		constexpr auto VERSION_MAJOR = 0;
		constexpr auto VERSION_MINOR = 0;
		constexpr auto VERSION_PATCH = 0;
		constexpr auto VERSION_BUILD = 0;
		constexpr auto IS_DEV = true;
	}

	// helper for variant visits, see: https://en.cppreference.com/w/cpp/utility/variant/visit
	template<typename... Ts>
	struct visitor : Ts... { using Ts::operator()...; };
	template<typename... Ts> visitor(Ts...)->visitor<Ts...>;
}