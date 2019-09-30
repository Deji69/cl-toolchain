#pragma once
#include <catch.hpp>
#include <initializer_list>
#include <CLARA/Compiler.h>
#include <CLARA/IBinaryOutput.h>
#include <CLARA/Parser.h>
#include <CLARA/Source.h>

using namespace CLARA;
using namespace CLARA::CLASM;

struct MockOutputHandler : public IBinaryOutput {
	vector<uint8_t> output;

	virtual ~MockOutputHandler() = default;

	template<typename T>
	auto check(const T& cont) const->bool
	{
		if (cont.size() != output.size()) {
			UNSCOPED_INFO(fmt::format("Size is {}, expected {}", output.size(), cont.size()));
			return false;
		}
		
		auto it = output.cbegin();
		
		for (auto expectIt = cont.begin(); expectIt != cont.end(); ++expectIt, ++it) {
			if (*expectIt != *it) {
				UNSCOPED_INFO(fmt::format(
					"Mismatch output[{}] == {:#x}, expected {:#x}",
					std::distance(output.cbegin(), it),
					*it,
					*expectIt
				));
				return false;
			}
		}
		return true;
	}

	virtual auto write8(uint8_t val)->void override
	{
		output.push_back(val);
	}

	virtual auto write16(uint16_t val)->void override
	{
		write8(reinterpret_cast<char*>(&val)[0]);
		write8(reinterpret_cast<char*>(&val)[1]);
	}

	virtual auto write32(uint32_t val)->void override
	{
		write8(reinterpret_cast<char*>(&val)[0]);
		write8(reinterpret_cast<char*>(&val)[1]);
		write8(reinterpret_cast<char*>(&val)[2]);
		write8(reinterpret_cast<char*>(&val)[3]);
	}

	virtual auto write(string_view str)->void override
	{
		for (auto c : str) {
			write8(c);
		}
	}
};