#pragma once

#ifdef _MSC_VER
#define _CRT_RAND_S
#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#define NOMINMAX
#pragma warning(push, 0)
#endif

// C library
#include <cassert>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
// C++ STL
#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>
// C++17
#include <any>
#include <filesystem>
#include <optional>
#include <string_view>
#include <variant>
// C++20
// #define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <fmt/color.h>

#ifdef _MSC_VER
#pragma warning(pop)
#undef CDECL
#endif