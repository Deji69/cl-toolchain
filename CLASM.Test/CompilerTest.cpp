#include "catch.hpp"
#include <string_view>
#include <CLASM/IBinaryOutput.h>
#include <CLASM/Compiler.h>

using namespace CLARA;
using namespace CLARA::CLASM;

/*class MockIBinaryOutput : public IBinaryOutput {
public:
	MOCK_METHOD1_T(writeU8, void(uint8));
	MOCK_METHOD1_T(writeU16, void(uint16));
	MOCK_METHOD1_T(writeU32, void(uint32));
	MOCK_METHOD1_T(writeI8, void(int8));
	MOCK_METHOD1_T(writeI16, void(int16));
	MOCK_METHOD1_T(writeI32, void(int32));
	MOCK_METHOD1_T(write, void(std::string_view));
};

TEST_CASE("compiles nop instruction", "[Compile]")
{
	MockIBinaryOutput out;
	LexerTokenStream tokens;
	Compiler compiler;
	tokens.push(Token::Instruction, Instruction::NOP);
	EXPECT_CALL(out, writeU8(uint{0x0}));
	compiler.compile(tokens, out);
}*/