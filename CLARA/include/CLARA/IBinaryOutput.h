#pragma once
#include <CLARA/Common/Imports.h>

namespace CLARA::CLASM {

// Interface for file output
class IBinaryOutput {
public:
	virtual ~IBinaryOutput() = default;

	virtual auto write8(uint8_t)->void = 0;
	virtual auto write16(uint16_t)->void = 0;
	virtual auto write32(uint32_t)->void = 0;
	virtual auto write64(uint64_t)->void = 0;
	virtual auto write(string_view)->void = 0;
};

}