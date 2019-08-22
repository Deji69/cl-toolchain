#pragma once
#include <CLARA/Common/Imports.h>

namespace CLARA {

template<typename It, typename T>
constexpr auto findIndex(It begin, It end, const T& value) {
	auto it = std::find(begin, end, value);
	return std::distance(begin, it);
}

template<typename It, typename UnaryPredicate>
constexpr auto findIndexIf(It begin, It end, UnaryPredicate func) {
	auto it = std::find_if(begin, end, func);
	return std::distance(begin, it);
}

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