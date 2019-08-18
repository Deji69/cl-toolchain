#include "catch.hpp"
#include <CLARA/Source.h>
#include <CLARA/Parser.h>

using namespace CLARA;
using namespace CLARA::CLASM;

auto tokenize(const string& code) {
	auto options = Parser::Options{};
	options.errorReporting = false;
	auto source = make_shared<Source>("test", code);
	return Parser::tokenize(options, source);
}

TEST_CASE("lexing", "[Parser]")
{
	SECTION("lexes whitespace")
	{
		auto res = tokenize(" \t\n\t");
		REQUIRE(res.ok());

		auto& tokens = *res.info.tokens;
		REQUIRE(tokens.size() == 1_uz);
		REQUIRE(tokens[0].type == TokenType::EOL);
		REQUIRE(tokens[0].text.empty());
	}

	SECTION("lexes newlines")
	{
		auto res = tokenize("\r\n\r\n");
		REQUIRE(res.ok());

		auto& tokens = *res.info.tokens;
		REQUIRE(tokens.size() == 1_uz);
		REQUIRE(tokens[0].type == TokenType::EOL);
	}

	SECTION("lexes comments")
	{
		auto res = tokenize("nop; comment here\n ; another comment\nnop");
		REQUIRE(res.ok());

		auto& tokens = *res.info.tokens;
		REQUIRE(tokens.size() == 3_uz);
		REQUIRE(tokens[0].type == TokenType::Instruction);
		REQUIRE(tokens[1].type == TokenType::Instruction);
		REQUIRE(tokens[2].type == TokenType::EOL);
	}

	SECTION("lexes labels")
	{
		auto res = tokenize("label: foo");
		auto& tokens = *res.info.tokens;
		REQUIRE(tokens.size() == 3uz);
		CHECK(tokens[0].type == TokenType::Label);
		CHECK(tokens[1].type == TokenType::Identifier);
	}
}

TEST_CASE("lexes separators", "[Parser]")
{
	auto res = tokenize("separated:identifiers");
	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() == 4_uz);
	CHECK(tokens[0].type == TokenType::Identifier);
	CHECK(tokens[1].type == TokenType::Separator);
	CHECK(tokens[2].type == TokenType::Identifier);
}

TEST_CASE("lexes numberic values", "[Parser]")
{
	auto res = tokenize("123 3.14 -12 -12.4 1.e-4");
	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() == 6_uz);
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

TEST_CASE("lexes segments", "[Parser]")
{
	auto res = tokenize(".segment");
	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() == 2_uz);
	CHECK(tokens[0].type == TokenType::Segment);
}

TEST_CASE("lexes strings", "[Parser]")
{
	auto res = tokenize("\"hello world\" \"here is\\\\\\\" a \\\"quoted\\\" string\" not_a_string");
	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() == 4_uz);
	CHECK(tokens[0].type == TokenType::String);
	CHECK(tokens[1].type == TokenType::String);
	CHECK(tokens[2].type == TokenType::Identifier);
}

TEST_CASE("parses segments", "[Parser]")
{
	auto res = tokenize(".data\n.code");
	REQUIRE(res.ok());
	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() == 3_uz);
	CHECK(tokens[0].type == TokenType::Segment);
	CHECK(tokens[1].type == TokenType::Segment);
	REQUIRE(is<Segment::Type>(tokens[0].annotation));
	REQUIRE(is<Segment::Type>(tokens[2].annotation));
	CHECK(get<Segment::Type>(tokens[0].annotation) == Segment::Data);
	CHECK(get<Segment::Type>(tokens[2].annotation) == Segment::Code);
}