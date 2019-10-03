#pragma once
#include <CLARA/Common/Imports.h>

namespace CLARA::CLASM {

// Interface for file output
class IBinaryOutput {
public:
	virtual ~IBinaryOutput() = default;

	virtual auto write(const uint8_t*, const uint8_t*)->void = 0;
	
	auto write8(uint8_t val)->void
	{
		write(&val, &val + 1);
	}

	auto write16(uint16_t val)->void
	{
		write(reinterpret_cast<uint8_t*>(&val), reinterpret_cast<uint8_t*>(&val + 1));
	}

	auto write32(uint32_t val)->void
	{
		write(reinterpret_cast<uint8_t*>(&val), reinterpret_cast<uint8_t*>(&val + 1));
	}

	auto write64(uint64_t val)->void
	{
		write(reinterpret_cast<uint8_t*>(&val), reinterpret_cast<uint8_t*>(&val + 1));
	}
	
	auto write(string_view sv)->void
	{
		write(reinterpret_cast<const uint8_t*>(sv.data()), reinterpret_cast<const uint8_t*>(sv.data()) + sv.size());
	}
};

}