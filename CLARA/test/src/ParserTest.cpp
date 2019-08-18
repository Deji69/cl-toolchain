#include <catch.hpp>
#include <CLARA/Source.h>
#include <CLARA/Parser.h>

using namespace CLARA;
using namespace CLARA::CLASM;

template<typename T>
auto isOneOf(const T& value, initializer_list<T> oneOfThese)
{
	for (auto& one : oneOfThese) {
		if (value == one)
			return true;
	}
	return false;
}

auto checkResult(const Parser::Result& res)
{
	for (auto& report : res.reports) {
		UNSCOPED_INFO(string{report.diagnosis.getName()} + " ["s + to_string(report.diagnosis.getCodeInt()) + "]: "s + string{report.token.text});
	}
	return res.ok();
}

auto getParseOpts(bool forceTokenization = false)
{
	// disabling reporting errors to stdout/stderr as it will mess with test results
	auto options = Parser::Options{};
	options.errorReporting = false;
	options.testForceTokenization = forceTokenization;
	return options;
}

struct ParsingTestHelper {
	Parser::Options options;
	shared_ptr<TokenStream> tokensPtr;

	ParsingTestHelper(bool forceTokenization = false) : options(getParseOpts(forceTokenization))
	{ }

	auto parse(string code)
	{
		return Parser::tokenize(options, make_shared<Source>("test", code));
	}

	auto& parseExpect(string code, initializer_list<TokenType> types)
	{
		auto res = parse(code);
		tokensPtr = res.info.tokens;
		REQUIRE(tokensPtr->size() >= types.size());
		auto i = 0_uz;
		for (auto type : types) {
			REQUIRE((*tokensPtr)[i++].type == type);
		}
		return *tokensPtr;
	}
};

auto lexerHelper = ParsingTestHelper(true);
auto helper = ParsingTestHelper();

TEST_CASE("Parser enforces start of line tokens per segment", "[Parser]")
{
	SECTION("Default header segment expects a keyword or segment")
	{
		auto res = helper.parse("\"string\"");
		REQUIRE(res.numErrors == 1_uz);
		REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::ExpectedToken);
		CHECK(res.reports[0].diagnosis.get<DiagCode::ExpectedToken>().given == TokenType::String);
		REQUIRE(is<AnyOf>(res.reports[0].diagnosis.get<DiagCode::ExpectedToken>().expected));
		auto& expected = get<AnyOf>(res.reports[0].diagnosis.get<DiagCode::ExpectedToken>().expected);

		for (auto& expect : expected) {
			REQUIRE(is<TokenType>(expect));
			CHECK(isOneOf(get<TokenType>(expect), {TokenType::EOL, TokenType::Identifier, TokenType::Segment}));
		}
	}

	SECTION("Code segment expects an instruction, mnemonic, label or segment")
	{
		auto res = helper.parse(".code\n\"string\"");
		REQUIRE(res.numErrors == 1_uz);
		REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::ExpectedToken);
		CHECK(res.reports[0].diagnosis.get<DiagCode::ExpectedToken>().given == TokenType::String);
		REQUIRE(is<AnyOf>(res.reports[0].diagnosis.get<DiagCode::ExpectedToken>().expected));
		auto& expected = get<AnyOf>(res.reports[0].diagnosis.get<DiagCode::ExpectedToken>().expected);

		for (auto& expect : expected) {
			REQUIRE(is<TokenType>(expect));
			CHECK(isOneOf(get<TokenType>(expect), {TokenType::EOL, TokenType::Identifier, TokenType::Label, TokenType::Segment}));
		}
	}

	SECTION("Data segment expects a label or segment")
	{
		auto res = helper.parse(".data\n\"string\"");
		REQUIRE(res.numErrors == 1_uz);
		REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::ExpectedToken);
		CHECK(res.reports[0].diagnosis.get<DiagCode::ExpectedToken>().given == TokenType::String);
		REQUIRE(is<AnyOf>(res.reports[0].diagnosis.get<DiagCode::ExpectedToken>().expected));
		auto& expected = get<AnyOf>(res.reports[0].diagnosis.get<DiagCode::ExpectedToken>().expected);

		for (auto& expect : expected) {
			REQUIRE(is<TokenType>(expect));
			CHECK(isOneOf(get<TokenType>(expect), {TokenType::EOL, TokenType::Label, TokenType::Segment}));
		}
	}
}

TEST_CASE("Lexer tokenizes final newline", "[Lexer]")
{
	auto res = lexerHelper.parse("\r\n\r\n");
	REQUIRE(checkResult(res));

	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() == 1_uz);
	CHECK(tokens[0].type == TokenType::EOL);
}

TEST_CASE("Lexer skips whitespace", "[Lexer]")
{
	auto res = lexerHelper.parse(" \t\n\t");
	REQUIRE(checkResult(res));

	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() == 1_uz);
	CHECK(tokens[0].type == TokenType::EOL);
	CHECK(tokens[0].text.empty());
}

TEST_CASE("Lexer tokenizes segments", "[Lexer]")
{
	auto res = lexerHelper.parse(".code");
	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() >= 1_uz);
	REQUIRE(tokens[0].type == TokenType::Segment);
	REQUIRE(get<Segment::Type>(tokens[0].annotation) == Segment::Code);
}

TEST_CASE("Lexer tokenizes instruction", "[Lexer]")
{
	auto res = lexerHelper.parse("nop");
	REQUIRE(checkResult(res));

	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() >= 1_uz);
	CHECK(tokens[0].type == TokenType::Instruction);
	CHECK(get<Instruction::Type>(tokens[0].annotation) == Instruction::NOP);
}

TEST_CASE("Lexer skips comments", "[Lexer]")
{
	auto res = lexerHelper.parse("nop; comment here\n ; another comment\nnop");
	REQUIRE(checkResult(res));

	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() >= 3_uz);
	CHECK(tokens[0].type == TokenType::Instruction);
	CHECK(tokens[1].type == TokenType::Instruction);
}

TEST_CASE("Lexer tokenizes labels", "[Lexer]")
{
	auto res = lexerHelper.parse("label: nop");
	REQUIRE(checkResult(res));

	auto& tokens = *res.info.tokens;
	CHECK(tokens.size() >= 2_uz);
	CHECK(tokens[0].type == TokenType::Label);
	CHECK(tokens[1].type == TokenType::Instruction);
}

TEST_CASE("Lexer tokenizes separated instructions", "[Lexer]")
{
	lexerHelper.parseExpect("nop,nop", {TokenType::Instruction, TokenType::Instruction});
}

TEST_CASE("Lexer tokenizes numerics", "[Lexer]")
{
	SECTION("tokenizes decimals") {
		auto& tokens = lexerHelper.parseExpect("123 3.14 -12 -12.4 1.e-4", {
			TokenType::Numeric, TokenType::Numeric, TokenType::Numeric,
			TokenType::Numeric, TokenType::Numeric
		});
		
		CHECK(tokens[0].text == "123");
		CHECK(tokens[1].text == "3.14");
		CHECK(tokens[2].text == "-12");
		CHECK(tokens[3].text == "-12.4");
		CHECK(tokens[4].text == "1.e-4");
	}
	
	SECTION("tokenizes hexadecimals") {
		auto& tokens = lexerHelper.parseExpect("0x0 0x1 0x10 0xFF 0x100", {
			TokenType::Numeric, TokenType::Numeric, TokenType::Numeric,
			TokenType::Numeric, TokenType::Numeric
		});
		
		CHECK(tokens[0].text == "0x0");
		CHECK(tokens[1].text == "0x1");
		CHECK(tokens[2].text == "0x10");
		CHECK(tokens[3].text == "0xFF");
		CHECK(tokens[4].text == "0x100");
	}
}

TEST_CASE("Lexer tokenizes strings", "[Lexer]")
{
	lexerHelper.parseExpect("\"hello world\"\nlabel: \"here is\\\\\\\" a \\\"quoted\\\" string\" not_a_string", {
		TokenType::String, TokenType::Label, TokenType::String, TokenType::Identifier
	});
}

TEST_CASE("Parser parses strings with hex escape sequences", "[Parser]") {
	SECTION("parses a single byte hex pair") {
		auto& tokens = helper.parseExpect("\"\\x41\"", {TokenType::String});
		REQUIRE(get<string>(tokens[0].annotation) == "A");
	}
	
	SECTION("parses a single byte hex digit") {
		auto& tokens = helper.parseExpect("\"\\x9\"", {TokenType::String});
		REQUIRE(get<string>(tokens[0].annotation) == "\x09");
	}
	
	SECTION("parses a two byte hex sequence") {
		auto& tokens = helper.parseExpect("\"\\x4142\"", {TokenType::String});
		REQUIRE(get<string>(tokens[0].annotation) == "AB");
	}
	
	SECTION("parses a three byte hex sequence") {
		auto& tokens = helper.parseExpect("\"\\x414243\"", {TokenType::String});
		REQUIRE(get<string>(tokens[0].annotation) == "ABC");
	}
	
	SECTION("parses a four byte hex sequence") {
		auto& tokens = helper.parseExpect("\"\\x41424344\"", {TokenType::String});
		REQUIRE(get<string>(tokens[0].annotation) == "ABCD");
	}

	SECTION("parsing stops at backslash to prevent interpreting extra characters as bytes") {
		auto& tokens = helper.parseExpect("\"\\x4142\\CD\"", {TokenType::String});
		REQUIRE(get<string>(tokens[0].annotation) == "ABCD");
	}
}

TEST_CASE("Parser parses numerics", "[Parser]") {
	SECTION("parses decimals") {
		auto& tokens = lexerHelper.parseExpect("123 3.14 -12 -12.4 1.e-4", {
			TokenType::Numeric, TokenType::Numeric, TokenType::Numeric,
			TokenType::Numeric, TokenType::Numeric
		});
		
		CHECK(tokens[0].text == "123");
		CHECK(tokens[1].text == "3.14");
		CHECK(tokens[2].text == "-12");
		CHECK(tokens[3].text == "-12.4");
		CHECK(tokens[4].text == "1.e-4");
	}
	
	SECTION("tokenizes hexadecimals") {
		auto& tokens = lexerHelper.parseExpect("0x0 0x1 0x10 0xFF 0x100", {
			TokenType::Numeric, TokenType::Numeric, TokenType::Numeric,
			TokenType::Numeric, TokenType::Numeric
		});
		
		CHECK(tokens[0].text == "0x0");
		CHECK(tokens[1].text == "0x1");
		CHECK(tokens[2].text == "0x10");
		CHECK(tokens[3].text == "0xFF");
		CHECK(tokens[4].text == "0x100");
	}
}

TEST_CASE("Parser parses labels", "[Parser]")
{
	SECTION("Labels can be defined")
	{
		auto res = helper.parse(".code\nlabel:");
		auto& tokens = *res.info.tokens;
		auto& labels = res.info.labels;
		auto& labelMap = res.info.labelMap;
		REQUIRE(tokens.size() == 3);
		auto& labelToken = tokens[1];
		REQUIRE(labelToken.is(TokenType::Label, "label:"));
		CHECK(is<const Label*>(labelToken.annotation));
		auto it = labelMap.find("label");
		REQUIRE(it != labelMap.end());
		REQUIRE(it->second < labels.size());
		auto label = labels[it->second].get();
		CHECK(label == get<const Label*>(labelToken.annotation));
		CHECK(label->name == "label");
		CHECK(&label->definition == &labelToken);
	}

	SECTION("Labels can be referenced")
	{
		auto res = helper.parse(".code\nlabel: jmp label");
		REQUIRE(checkResult(res));
		auto& tokens = *res.info.tokens;
		REQUIRE(tokens.size() == 5);
		REQUIRE(is<const Label*>(tokens[3].annotation));
		auto ref = get<const Label*>(tokens[3].annotation);
		REQUIRE(ref == res.info.labels[0].get());
		REQUIRE(ref == get<const Label*>(tokens[1].annotation));
	}

	SECTION("Labels can be defined after referencing")
	{
		auto res = helper.parse(".code\njmp label\nlabel:");
		REQUIRE(checkResult(res));
		auto& tokens = *res.info.tokens;
		REQUIRE(tokens.size() == 5);
		REQUIRE(is<const Label*>(tokens[2].annotation));
		REQUIRE(is<const Label*>(tokens[3].annotation));
		auto ref = get<const Label*>(tokens[2].annotation);
		REQUIRE(ref == get<const Label*>(tokens[3].annotation));
	}
}

TEST_CASE("Parser resolves mnemonics", "[Parser]")
{
	auto res = helper.parse(".code\npush 0xFF\npush 0x100");
	REQUIRE(checkResult(res));
}

TEST_CASE("Error unexpected lexeme", "[Error Handling]")
{
	auto res = lexerHelper.parse("`123");
	REQUIRE(res.hadFatal);
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::UnexpectedLexeme);
}

TEST_CASE("Error expected EOL after segment", "[Error Handling]")
{
	auto res = helper.parse(".code 1");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::ExpectedToken);
	CHECK(res.reports[0].diagnosis.get<DiagCode::ExpectedToken>().given == TokenType::Numeric);
	auto& diagnosis = res.reports[0].diagnosis.get<DiagCode::ExpectedToken>();
	CHECK(get<TokenType>(diagnosis.expected) == TokenType::EOL);
}

TEST_CASE("Error unexpected separator", "[Error Handling]")
{
	auto res = lexerHelper.parse(":");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::UnexpectedSeparator);
}

TEST_CASE("Error unexpected segment after tokens", "[Error Handling]")
{
	auto res = lexerHelper.parse("nop .code");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::UnexpectedSegmentAfterTokens);
}

TEST_CASE("Error unexpected label after tokens", "[Error Handling]")
{
	// parser picks up 'label' as an identifier (possible label pointer) and then errors on the ':' - this is ok?
	auto res = lexerHelper.parse(".code\nnop label:");
	REQUIRE(res.numErrors >= 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::UnexpectedLabelAfterTokens);
}

TEST_CASE("Error invalid identifier", "[Error Handling]")
{
	auto res = helper.parse("blahdyblahbloo");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::InvalidIdentifier);
}

TEST_CASE("Error instruction passed as operand", "[Error Handling]")
{
	auto res = lexerHelper.parse("nop nop");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::UnexpectedOperand);
	CHECK(res.reports[0].diagnosis.getMessage() == "unexpected instruction encountered, use ',' to separate multiple instructions on one line"s);
}

TEST_CASE("Error instruction missing operand", "[Error Handling]")
{
	auto res = lexerHelper.parse("pushb");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::MissingOperand);
}

TEST_CASE("Error instruction invalid operand type", "[Error Handling]")
{
	auto res = lexerHelper.parse("pushb \"str\"");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::InvalidOperandType);
}

TEST_CASE("Error undefined label", "[Error Handling]")
{
	auto res = helper.parse(".code\njmp label");
	REQUIRE(res.numErrors >= 1);
	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() >= 3);
	REQUIRE(tokens[2].type == TokenType::Label);
	REQUIRE(is<string>(tokens[2].annotation));
	CHECK(get<string>(tokens[2].annotation) == "label");
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::UnresolvedLabelReference);
}
