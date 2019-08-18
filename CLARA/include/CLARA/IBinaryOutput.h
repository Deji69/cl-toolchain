#pragma once
#include <CLARA/Common/Imports.h>

namespace CLARA::CLASM {

// Interface for file output
class IBinaryOutput {
public:
	virtual ~IBinaryOutput() = default;

	virtual auto writeU8(uint8_t)->void = 0;
	virtual auto writeU16(uint16_t)->void = 0;
	virtual auto writeU32(uint32_t)->void = 0;
	virtual auto writeI8(int8_t)->void = 0;
	virtual auto writeI16(int16_t)->void = 0;
	virtual auto writeI32(int32_t)->void = 0;
	virtual auto write(string_view)->void = 0;
};

}