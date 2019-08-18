#pragma once
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <any>
#include <array>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <fmt/format.h>
#include <fmt/color.h>

#ifdef _MSC_VER
#pragma warning(pop)
#undef CDECL
#endif

namespace CLARA {

namespace fs {
using namespace std::filesystem;
}

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;
namespace chrono = std::chrono;

using uint = unsigned int;
using int8 = int8_t;
using uint8 = uint8_t;
using int16 = int16_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;

using std::any;
using std::array;
using std::function;
using std::ifstream;
using std::initializer_list;
using std::list;
using std::map;
using std::monostate;
using std::nullopt;
using std::optional;
using std::pair;
using std::reference_wrapper;
using std::set;
using std::shared_ptr;
using std::stack;
using std::string;
using std::string_view;
using std::stringstream;
using std::tuple;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::variant;
using std::vector;
using std::weak_ptr;
using std::wstring;

template<typename T, typename... Types>
inline constexpr auto is(const std::variant<Types...>& var) noexcept->bool
{
	return std::holds_alternative<T>(var);
}

using std::forward;
using std::forward_as_tuple;
using std::get;
using std::get_if;
using std::make_move_iterator;
using std::make_optional;
using std::make_pair;
using std::make_shared;
using std::make_unique;
using std::move;
using std::tie;

template<typename T, std::enable_if_t<std::is_scalar_v<T>, int> = 0>
inline constexpr auto to_string(T val)
{
	return std::to_string(val);
}

}
