#pragma once
#include <CLASM/Common/Imports.h>

namespace CLARA {

constexpr auto operator ""_i8(unsigned long long v)->std::int8_t
{
	return static_cast<std::int8_t>(v);
}

constexpr auto operator ""_u8(unsigned long long v)->std::uint8_t
{
	return static_cast<std::uint8_t>(v);
}

constexpr auto operator ""_i16(unsigned long long v)->std::int16_t
{
	return static_cast<std::int16_t>(v);
}

constexpr auto operator ""_u16(unsigned long long v)->std::uint16_t
{
	return static_cast<std::uint16_t>(v);
}

constexpr auto operator ""_i32(unsigned long long v)->std::int32_t
{
	return static_cast<std::int32_t>(v);
}

constexpr auto operator ""_u32(unsigned long long v)->std::uint32_t
{
	return static_cast<std::uint32_t>(v);
}

constexpr auto operator ""_i64(unsigned long long v)->std::int64_t
{
	return static_cast<std::int64_t>(v);
}

constexpr auto operator ""_u64(unsigned long long v)->std::int64_t
{
	return static_cast<std::uint64_t>(v);
}

constexpr auto operator ""_uz(unsigned long long v)->std::size_t
{
	return static_cast<std::size_t>(v);
}

constexpr auto operator ""_z(unsigned long long v)->std::ptrdiff_t
{
	return static_cast<std::ptrdiff_t>(v);
}

}