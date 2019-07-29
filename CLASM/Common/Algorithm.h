#pragma once
#include <CLASM/Common/Imports.h>

namespace CLARA {

template<typename TRetType, typename TCont>
constexpr auto findOpt(const TCont& cont, const typename TCont::key_type& key)->
	std::enable_if_t<!std::is_same_v<TRetType, typename TCont::mapped_type>, optional<TRetType>>
{
	auto it = cont.find(key);
	return it != cont.end() ? make_optional<TRetType>(it->second) : nullopt;
}

template<typename TCont>
constexpr auto findOpt(const TCont& cont, const typename TCont::key_type& key)->optional<typename TCont::mapped_type>
{
	auto it = cont.find(key);
	return it != cont.end() ? make_optional(it->second) : nullopt;
}

}