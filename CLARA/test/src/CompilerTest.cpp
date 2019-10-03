#include "catch.hpp"
#include <string_view>
#include <CLARA/Compiler.h>
#include <CLARA/IBinaryOutput.h>
#include "CompilerHelper.h"

using namespace CLARA;
using namespace CLARA::CLASM;

auto compile(initializer_list<TokenAnnotation> input)
{
	MockOutputHandler out;
	const auto parsed = makeParseInfo(input);
	Compiler::Options opts;
	Compiler::compile(opts, parsed, out);
	return out;
}

auto compileCheck(initializer_list<TokenAnnotation> input, initializer_list<uint8_t> expect)
{
	return compile(input).check(expect);
}

TEST_CASE("compiles nop instruction", "[Compile]")
{
	REQUIRE(compileCheck(
		{Segment::Code, Instruction::NOP},
		{Instruction::NOP}
	));
}

TEST_CASE("compiles pushb instruction", "[Compile]")
{
	REQUIRE(compileCheck(
		{Segment::Code, Instruction::PUSHB, 50_u8},
		{Instruction::PUSHB, 50_u8}
	));
}