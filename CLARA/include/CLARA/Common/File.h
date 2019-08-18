#pragma once
#include <CLARA/Common/Imports.h>

namespace CLARA::CLASM {

namespace fs {

// convenience function for non-throwing fs::exists
inline auto fexists(const fs::path& path) noexcept->bool
{
	std::error_code ec{};
	return exists(path, ec);
}

}

inline auto readBinaryFile(const fs::path& path)->optional<vector<uint8>>
{
	ifstream file(path, std::ios::binary);

	if (file.is_open()) {
		vector<uint8> buf;
		buf.reserve(fs::file_size(path));
		buf.assign(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});
		return buf;
	}

	return nullopt;
}

}