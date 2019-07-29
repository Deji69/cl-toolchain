#include "catch.hpp"
#include <CLASM/Source.h>
#include <CLASM/Parser.h>

using namespace CLARA;
using namespace CLARA::CLASM;

auto checkResult(const ParserResult& res)
{
	for (auto& report : res.reports) {
		UNSCOPED_INFO(string{report.diagnosis.getName()} + " ["s + to_string(report.diagnosis.getCodeInt()) + "]: "s + string{report.token.text});
	}
	return res.ok();
}

auto getParser()
{
	// disabling reporting errors to stdout/stderr as it will mess with test results
	auto options = ParserOptions{};
	options.errorReporting = false;
	return Parser{options};
}

struct ParsingTestHelper {
	Parser parser;
	shared_ptr<TokenStream> tokensPtr;

	ParsingTestHelper()
	{
		parser = getParser();
	}

	auto parse(string code)
	{
		return parser.tokenize(make_shared<Source>("test", code));
	}

	auto& parseExpect(string code, initializer_list<TokenType> types)
	{
		auto res = parse(code);
		tokensPtr = res.tokens;
		REQUIRE(tokensPtr->size() >= types.size());
		auto i = size_t{0};
		for (auto type : types) {
			REQUIRE((*tokensPtr)[i].type == type);
		}
		return *tokensPtr;
	}
};

TEST_CASE("Parser tokenizes whitespace")
{
	auto source = make_shared<Source>("test", " \t\n\t");
	auto res = getParser().tokenize(source);
	REQUIRE(checkResult(res));

	auto& tokens = *res.tokens;
	REQUIRE(tokens.size() == size_t{1});
	CHECK(tokens[0].type == TokenType::EOL);
	CHECK(tokens[0].text.empty());
}

TEST_CASE("Parser tokenizes newlines")
{
	auto source = make_shared<Source>("test", "\r\n\r\n");
	auto res = getParser().tokenize(source);
	REQUIRE(checkResult(res));

	auto& tokens = *res.tokens;
	REQUIRE(tokens.size() == size_t{1});
	CHECK(tokens[0].type == TokenType::EOL);
}

TEST_CASE("Parser tokenizes comments")
{
	auto source = make_shared<Source>("test", "nop; comment here\n ; another comment\nnop");
	auto res = getParser().tokenize(source);
	REQUIRE(checkResult(res));

	auto& tokens = *res.tokens;
	REQUIRE(tokens.size() == size_t{3});
	CHECK(tokens[0].type == TokenType::Instruction);
	CHECK(tokens[1].type == TokenType::Instruction);
	CHECK(tokens[2].type == TokenType::EOL);
}

TEST_CASE("Parser tokenizes instruction")
{
	auto source = make_shared<Source>("test", "nop");
	auto res = getParser().tokenize(source);
	REQUIRE(checkResult(res));

	auto& tokens = *res.tokens;
	REQUIRE(tokens.size() == size_t{2});
	CHECK(tokens[0].type == TokenType::Instruction);
	CHECK(get<Instruction::Type>(tokens[0].annotation) == Instruction::NOP);
}

TEST_CASE("Parser tokenizes labels")
{
	auto source = make_shared<Source>("test", "label: nop");
	auto res = getParser().tokenize(source);
	REQUIRE(checkResult(res));

	auto& tokens = *res.tokens;
	CHECK(tokens.size() == size_t{3});
	CHECK(tokens[0].type == TokenType::Label);
	CHECK(tokens[1].type == TokenType::Instruction);
}

TEST_CASE("Parser tokenizes separated instructions")
{
	auto source = make_shared<Source>("test", "nop,nop");
	auto res = getParser().tokenize(source);
	REQUIRE(checkResult(res));

	auto& tokens = *res.tokens;
	REQUIRE(tokens.size() == size_t{3});
	CHECK(tokens[0].type == TokenType::Instruction);
	CHECK(tokens[1].type == TokenType::Instruction);
}

TEST_CASE("Parser tokenizes numerics")
{
	auto source = make_shared<Source>("test", "123 3.14 -12 -12.4 1.e-4");
	auto res = getParser().tokenize(source);

	auto& tokens = *res.tokens;
	REQUIRE(tokens.size() == size_t{6});
	CHECK(tokens[0].type == TokenType::Numeric);
	CHECK(tokens[0].text == "123");
	CHECK(tokens[1].type == TokenType::Numeric);
	CHECK(tokens[1].text == "3.14");
	CHECK(tokens[2].type == TokenType::Numeric);
	CHECK(tokens[2].text == "-12");
	CHECK(tokens[3].type == TokenType::Numeric);
	CHECK(tokens[3].text == "-12.4");
	CHECK(tokens[4].type == TokenType::Numeric);
	CHECK(tokens[4].text == "1.e-4");
}

TEST_CASE("Parser tokenizes segments")
{
	auto source = make_shared<Source>("test", ".code");
	auto res = getParser().tokenize(source);
	auto& tokens = *res.tokens;
	REQUIRE(tokens.size() == size_t{2});
	REQUIRE(tokens[0].type == TokenType::Segment);
	REQUIRE(get<Segment::Type>(tokens[0].annotation) == Segment::Code);
}

TEST_CASE("Parser tokenizes strings")
{
	auto source = make_shared<Source>("test", "\"hello world\" \"here is\\\\\\\" a \\\"quoted\\\" string\" not_a_string");
	auto res = getParser().tokenize(source);
	auto& tokens = *res.tokens;
	REQUIRE(tokens.size() >= size_t{3});
	REQUIRE(tokens[0].type == TokenType::String);
	REQUIRE(tokens[1].type == TokenType::String);
	REQUIRE(tokens[2].type == TokenType::Identifier);
}

TEST_CASE("parses strings with hex escape sequences")
{
	ParsingTestHelper helper;

	SECTION("parses a single byte hex pair")
	{
		auto& tokens = helper.parseExpect("\"\\x41\"", {TokenType::String});
		REQUIRE(get<string>(tokens[0].annotation) == "A");
	}
	
	SECTION("parses a single byte hex digit")
	{
		auto& tokens = helper.parseExpect("\"\\x9\"", {TokenType::String});
		REQUIRE(get<string>(tokens[0].annotation) == "\x09");
	}
	
	SECTION("parses a two byte hex sequence")
	{
		auto& tokens = helper.parseExpect("\"\\x4142\"", {TokenType::String});
		REQUIRE(get<string>(tokens[0].annotation) == "AB");
	}
	
	SECTION("parses a three byte hex sequence")
	{
		auto& tokens = helper.parseExpect("\"\\x414243\"", {TokenType::String});
		REQUIRE(get<string>(tokens[0].annotation) == "ABC");
	}
	
	SECTION("parses a four byte hex sequence")
	{
		auto& tokens = helper.parseExpect("\"\\x41424344\"", {TokenType::String});
		REQUIRE(get<string>(tokens[0].annotation) == "ABCD");
	}

	SECTION("parsing stops at backslash to prevent interpreting extra characters as bytes")
	{
		auto& tokens = helper.parseExpect("\"\\x4142\\CD\"", {TokenType::String});
		REQUIRE(get<string>(tokens[0].annotation) == "ABCD");
	}
}

TEST_CASE("error reporting")
{
	ParsingTestHelper helper;

	SECTION("reports ")
	{
	}
}