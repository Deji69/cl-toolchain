#pragma once
#include <CLARA/Common/Imports.h>

namespace CLARA {

template<typename FwdIt>
inline auto join(FwdIt begin, FwdIt end, string_view delim = ",")->string
{
	if (begin == end) return ""s;
	auto sdelim = string{delim};
	return std::accumulate(begin + 1, end, to_string(*begin), [sdelim](const string& str, auto val) {
		return str + sdelim + to_string(val);
	});
}

template<typename Cont>
auto join(Cont cont, string_view delim = ",")->string
{
	return join(std::begin(cont), std::end(cont), delim);
}

auto ltrim(string_view subj, string_view::const_pointer str = " \t\n\r")->string_view;
auto rtrim(string_view subj, string_view::const_pointer str = " \t\n\r")->string_view;
auto trim(string_view subj, string_view::const_pointer str = " \t\n\r")->string_view;
auto ltrimmed(string_view subj, string_view::const_pointer str = " \t\n\r")->string;
auto rtrimmed(string_view subj, string_view::const_pointer str = " \t\n\r")->string;
auto trimmed(string_view subj, string_view::const_pointer str = " \t\n\r")->string;

auto tokenize(vector<string_view>& tokens_out, string_view subj, size_t limit = 0, string_view::const_pointer delims = ",", string_view::const_pointer trim = " \t")->size_t;

template<typename TNum, std::enable_if_t<std::is_integral_v<TNum>, int> = 0>
constexpr auto stringToNumber(string_view sv, int base = 10)->optional<TNum>
{
	TNum value;
	auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value, base);
	return ec == std::errc{} ? make_optional(value) : nullopt;
}

template<typename TNum, std::enable_if_t<std::is_floating_point_v<TNum>, int> = 0>
constexpr auto stringToNumber(string_view sv, std::chars_format fmt)->optional<TNum>
{
	TNum value;
	auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value, fmt);
	return ec == std::errc{} ? make_optional(value) : nullopt;
}

template<typename TInt = int, std::enable_if_t<std::is_integral_v<TInt>, int> = 0>
constexpr auto stringToInt(string_view sv, int base = 10)->optional<TInt>
{
	return stringToNumber<TInt>(sv, base);
}

template<typename TFlt = float, std::enable_if_t<std::is_floating_point_v<TFlt>, int> = 0>
constexpr auto stringToFloat(string_view sv)->optional<TFlt>
{
	return stringToNumber<TFlt>(sv);
}

template<typename TFlt = double, std::enable_if_t<std::is_floating_point_v<TFlt>, int> = 0>
constexpr auto stringToDouble(string_view sv)->optional<TFlt>
{
	return stringToNumber<TFlt>(sv);
}

auto stringToLower(string& str)->string&;
auto stringToUpper(string& str)->string&;
auto stringLower(string str)->string;
auto stringUpper(string str)->string;

auto stringReplace(string& subj, const string& find, const string& repl)->string&;
auto stringReplaced(string subj, const string& find, const string& repl)->string;

auto stringBeginsWith(string_view s, string_view::value_type c)->bool;
auto stringBeginsWith(string_view s, const string& v)->bool;
auto stringEndsWith(string_view s, string_view::value_type c)->bool;
auto stringEndsWith(string_view s, const string& v)->bool;

template<typename TInt, size_t TSize = sizeof(TInt)>
constexpr auto encodeBytesLE(TInt value)->std::enable_if_t<std::is_integral_v<TInt> && TSize <= sizeof(TInt), std::array<char, TSize>>
{
	auto arr = std::array<char, TSize>{};
	std::reverse_copy(reinterpret_cast<uint8*>(&value), reinterpret_cast<uint8*>(&value) + TSize, arr.data());
	return arr;
}

template<typename TInt, size_t TSize = sizeof(TInt)>
constexpr auto encodeBytesBE(TInt value)->std::enable_if_t<std::is_integral_v<TInt> && TSize <= sizeof(TInt), std::array<char, TSize>>
{
	auto arr = std::array<char, TSize>{};
	std::copy(reinterpret_cast<uint8*>(&value), reinterpret_cast<uint8*>(&value) + TSize, arr.data());
	return arr;
}

template<typename TInt, size_t TSize = sizeof(TInt)>
constexpr auto encodeBytes(TInt value)->std::enable_if_t<std::is_integral_v<TInt>&& TSize <= sizeof(TInt), std::array<char, TSize>>
{
	// C++20 feature supported in MSVC, wait for at least Clang support to remove preprocessor block
	// Unfortunately preprocessor blocks targetting the Clang compiler cannot be used when using Visual Studio with Clang set as the compiler
#if 0
	return std::endian::native == std::endian::little ? encodeBytesLE(value) : encodeBytesBE(value);
#else
	auto test = uint16_t{1};
	return *static_cast<unsigned char*>(static_cast<void*>(&test)) == 1
		? encodeBytesLE<TInt, TSize>(value)
		: encodeBytesBE<TInt, TSize>(value);
#endif
}

}
