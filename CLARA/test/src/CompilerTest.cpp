#include "catch.hpp"
#include <string_view>
#include <CLARA/Compiler.h>
#include <CLARA/IBinaryOutput.h>
#include "CompilerHelper.h"

using namespace CLARA;
using namespace CLARA::CLASM;

TEST_CASE("compiles nop instruction", "[Compile]")
{
	MockOutputHandler out;
	const auto expected = initializer_list<uint8_t>{0x90};
	auto ts = TokenStream({
		{TokenType::Instruction, Instruction::NOP}
	});
	Compiler::Options opts;
	Compiler::compile(opts, ts, out);
	REQUIRE(out.check(expected));
}