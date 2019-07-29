#include "catch.hpp"
#include <CLASM/Source.h>
#include <CLASM/Parser.h>

using namespace CLARA;
using namespace CLARA::CLASM;

TEST_CASE("lexing", "[Parser]")
{
	auto options = ParserOptions{};
	options.errorReporting = false;
	auto parser = Parser{options};

	SECTION("lexes whitespace")
	{
		auto source = make_shared<Source>("test", " \t\n\t");
		auto res = parser.tokenize(source);
		REQUIRE(res.ok());

		auto& tokens = *res.tokens;
		REQUIRE(tokens.size() == size_t{1});
		REQUIRE(tokens[0].type == TokenType::EOL);
		REQUIRE(tokens[0].text.empty());
	}

	SECTION("lexes newlines")
	{
		auto source = make_shared<Source>("test", "\r\n\r\n");
		auto res = parser.tokenize(source);
		REQUIRE(res.ok());

		auto& tokens = *res.tokens;
		REQUIRE(tokens.size() == size_t{1});
		REQUIRE(tokens[0].type == TokenType::EOL);
	}

	SECTION("lexes comments")
	{
		auto source = make_shared<Source>("test", "nop; comment here\n ; another comment\nnop");
		auto res = parser.tokenize(source);
		REQUIRE(res.ok());

		auto& tokens = *res.tokens;
		REQUIRE(tokens.size() == size_t{3});
		REQUIRE(tokens[0].type == TokenType::Instruction);
		REQUIRE(tokens[1].type == TokenType::Instruction);
		REQUIRE(tokens[2].type == TokenType::EOL);
	}

	SECTION("lexes labels")
	{
		auto source = make_shared<Source>("test", "label: foo");
		auto res = parser.tokenize(source);
		REQUIRE(res.ok());

		auto& tokens = *res.tokens;
		REQUIRE(tokens.size() == size_t{3});
		REQUIRE(tokens[0].type == TokenType::Label);
		REQUIRE(tokens[1].type == TokenType::Identifier);
	}
}

TEST_CASE("lexes separators", "[Parser]")
{
	auto source = make_shared<Source>("test", "separated:identifiers");
	auto res = Parser{}.tokenize(source);
	REQUIRE(res.ok());
	auto& tokens = *res.tokens;
	REQUIRE(tokens.size() == size_t{4});
	REQUIRE(tokens[0].type == TokenType::Identifier);
	REQUIRE(tokens[1].type == TokenType::Separator);
	REQUIRE(tokens[2].type == TokenType::Identifier);
}

TEST_CASE("lexes numberic values", "[Parser]")
{
	auto source = make_shared<Source>("test", "123 3.14 -12 -12.4 1.e-4");
	auto res = Parser{}.tokenize(source);
	REQUIRE(res.ok());
	auto& tokens = *res.tokens;
	REQUIRE(tokens.size() == size_t{6});
	REQUIRE(tokens[0].type == TokenType::Numeric);
	REQUIRE(tokens[0].text == "123");
	REQUIRE(tokens[1].type == TokenType::Numeric);
	REQUIRE(tokens[1].text == "3.14");
	REQUIRE(tokens[2].type == TokenType::Numeric);
	REQUIRE(tokens[2].text == "-12");
	REQUIRE(tokens[3].type == TokenType::Numeric);
	REQUIRE(tokens[3].text == "-12.4");
	REQUIRE(tokens[4].type == TokenType::Numeric);
	REQUIRE(tokens[4].text == "1.e-4");
}

TEST_CASE("lexes segments", "[Parser]")
{
	auto source = make_shared<Source>("test", ".segment");
	auto res = Parser{}.tokenize(source);
	REQUIRE(res.ok());
	auto& tokens = *res.tokens;
	REQUIRE(tokens.size() == size_t{2});
	REQUIRE(tokens[0].type == TokenType::Segment);
}

TEST_CASE("lexes strings", "[Parser]")
{
	auto source = make_shared<Source>("test", "\"hello world\" \"here is\\\\\\\" a \\\"quoted\\\" string\" not_a_string");
	auto res = Parser{}.tokenize(source);
	REQUIRE(res.ok());
	auto& tokens = *res.tokens;
	REQUIRE(tokens.size() == size_t{4});
	REQUIRE(tokens[0].type == TokenType::String);
	REQUIRE(tokens[1].type == TokenType::String);
	REQUIRE(tokens[2].type == TokenType::Identifier);
}

TEST_CASE("parses segments", "[Parser]")
{
	auto source = make_shared<Source>("test", ".data\n.code");
	auto res = Parser{}.tokenize(source);
	REQUIRE(res.ok());
	auto& tokens = *res.tokens;
	REQUIRE(tokens.size() == size_t{4});
	REQUIRE(tokens[0].type == TokenType::Segment);
	REQUIRE(tokens[2].type == TokenType::Segment);
	REQUIRE(is<Segment::Type>(tokens[0].annotation));
	REQUIRE(is<Segment::Type>(tokens[2].annotation));
	REQUIRE(get<Segment::Type>(tokens[0].annotation) == Segment::Data);
	REQUIRE(get<Segment::Type>(tokens[2].annotation) == Segment::Code);
}