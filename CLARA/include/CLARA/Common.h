#pragma once
#include <CLARA/Common/Macros.h>
#include <CLARA/Common/Imports.h>
#include <CLARA/Common/Algorithm.h>
#include <CLARA/Common/String.h>
#include <CLARA/Common/Literals.h>

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