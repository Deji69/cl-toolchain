#include <catch.hpp>
#include <CLARA/Source.h>
#include <CLARA/Parser.h>
#include "ParserHelper.h"

using namespace CLARA;
using namespace CLARA::CLASM;

auto lexerHelper = ParsingTestHelper(true);
auto helper = ParsingTestHelper();

auto parseAnnotation(string code, size_t idx) {
	idx += 1;
	auto res = helper.parse(".code\n" + code);
	REQUIRE(checkResult(res));
	REQUIRE(res.info.tokens->size() > idx);
	return (*res.info.tokens)[idx].annotation;
}

TEST_CASE("Lexer tokenizes EOF", "[Lexer]") {
	auto res = lexerHelper.parse("\r\n\r\n");
	REQUIRE(checkResult(res));

	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() == 1_uz);
	CHECK(tokens[0].type == TokenType::EndOfFile);
}

TEST_CASE("Lexer skips whitespace", "[Lexer]") {
	auto res = lexerHelper.parse(" \t\n\t");
	REQUIRE(checkResult(res));

	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() == 1_uz);
	CHECK(tokens[0].type == TokenType::EndOfFile);
	CHECK(tokens[0].text.empty());
}

TEST_CASE("Lexer tokenizes segments", "[Lexer]") {
	auto res = lexerHelper.parse(".code");
	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() >= 1_uz);
	REQUIRE(tokens[0].type == TokenType::Segment);
	REQUIRE(get<Segment::Type>(tokens[0].annotation) == Segment::Code);
}

TEST_CASE("Lexer tokenizes instruction", "[Lexer]") {
	auto res = lexerHelper.parse("nop");
	REQUIRE(checkResult(res));

	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() >= 1_uz);
	CHECK(tokens[0].type == TokenType::Instruction);
	CHECK(get<Instruction::Type>(tokens[0].annotation) == Instruction::NOP);
}

TEST_CASE("Lexer skips comments", "[Lexer]") {
	auto res = lexerHelper.parse("nop; comment here\n ; another comment\nnop");
	REQUIRE(checkResult(res));

	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() >= 3_uz);
	CHECK(tokens[0].type == TokenType::Instruction);
	CHECK(tokens[1].type == TokenType::Instruction);
}

TEST_CASE("Lexer tokenizes labels", "[Lexer]") {
	auto res = lexerHelper.parse("label: nop");
	REQUIRE(checkResult(res));

	auto& tokens = *res.info.tokens;
	CHECK(tokens.size() >= 2_uz);
	CHECK(tokens[0].type == TokenType::Label);
	CHECK(tokens[1].type == TokenType::Instruction);
}

TEST_CASE("Lexer tokenizes separated instructions", "[Lexer]") {
	lexerHelper.parseExpect("nop,nop", {TokenType::Instruction, TokenType::Instruction});
}

TEST_CASE("Lexer tokenizes numerics", "[Lexer]") {
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
		auto& tokens = lexerHelper.parseExpect("0x0 0x1 0x10 0xFF 0x100 -0x8F", {
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

TEST_CASE("Lexer tokenizes strings", "[Lexer]") {
	lexerHelper.parseExpect("\"hello world\"\nlabel: \"here is\\\\\\\" a \\\"quoted\\\" string\" not_a_string", {
		TokenType::String, TokenType::Label, TokenType::String, TokenType::Identifier
	});
}

TEST_CASE("Parser enforces start of line tokens per segment", "[Parser]") {
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
			CHECK(isOneOf(get<TokenType>(expect), {TokenType::EndOfLine, TokenType::EndOfFile, TokenType::Identifier, TokenType::Segment}));
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
			CHECK(isOneOf(get<TokenType>(expect), {TokenType::EndOfFile, TokenType::EndOfLine, TokenType::Identifier, TokenType::Label, TokenType::Segment}));
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
			CHECK(isOneOf(get<TokenType>(expect), {TokenType::EndOfFile, TokenType::EndOfLine, TokenType::Label, TokenType::Segment}));
		}
	}
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
		
		CHECK(is<int8>(tokens[0].annotation));
		CHECK(is<float>(tokens[1].annotation));
		CHECK(is<int8>(tokens[2].annotation));
		CHECK(is<float>(tokens[3].annotation));
		CHECK(is<float>(tokens[4].annotation));
	}
	
	SECTION("parses hexadecimals") {
		auto& tokens = lexerHelper.parseExpect("0x0 0x1 0x10 0x80 0xFF 0x100 0x8000 0xFFFF 0x10000 0x7FFFFFFF 0x80000000 0xFFFFFFFF", {
			TokenType::Numeric, TokenType::Numeric, TokenType::Numeric,
			TokenType::Numeric, TokenType::Numeric, TokenType::Numeric,
			TokenType::Numeric, TokenType::Numeric, TokenType::Numeric,
			TokenType::Numeric, TokenType::Numeric, TokenType::Numeric
		});
		
		REQUIRE(is<int8>(tokens[0].annotation));
		CHECK(get<int8>(tokens[0].annotation) == 0x0);
		REQUIRE(is<int8>(tokens[1].annotation));
		CHECK(get<int8>(tokens[1].annotation) == 0x1);
		REQUIRE(is<int8>(tokens[2].annotation));
		CHECK(get<int8>(tokens[2].annotation) == 0x10);
		REQUIRE(is<uint8>(tokens[3].annotation));
		CHECK(get<uint8>(tokens[3].annotation) == 0x80);
		REQUIRE(is<uint8>(tokens[4].annotation));
		CHECK(get<uint8>(tokens[4].annotation) == 0xFF);
		REQUIRE(is<uint8>(tokens[4].annotation));
		CHECK(get<uint8>(tokens[4].annotation) == 0xFF);
		REQUIRE(is<int16>(tokens[5].annotation));
		CHECK(get<int16>(tokens[5].annotation) == 0x100);
		REQUIRE(is<uint16>(tokens[6].annotation));
		CHECK(get<uint16>(tokens[6].annotation) == 0x8000);
		REQUIRE(is<uint16>(tokens[7].annotation));
		CHECK(get<uint16>(tokens[7].annotation) == 0xFFFF);
		REQUIRE(is<int32>(tokens[8].annotation));
		CHECK(get<int32>(tokens[8].annotation) == 0x10000);
		REQUIRE(is<int32>(tokens[9].annotation));
		CHECK(get<int32>(tokens[9].annotation) == 0x7FFFFFFF);
		REQUIRE(is<uint32>(tokens[10].annotation));
		CHECK(get<uint32>(tokens[10].annotation) == 0x80000000);
		REQUIRE(is<uint32>(tokens[11].annotation));
		CHECK(get<uint32>(tokens[11].annotation) == 0xFFFFFFFF);
	}
	
	SECTION("parses negated hexadecimals") {
		auto& tokens = lexerHelper.parseExpect("-0x0 -0x1 -0x7F -0x80 -0x7FF -0x8000 -0x7FFFFFFF -0x80000000", {
			TokenType::Numeric, TokenType::Numeric, TokenType::Numeric,
			TokenType::Numeric, TokenType::Numeric, TokenType::Numeric,
			TokenType::Numeric, TokenType::Numeric
		});
		REQUIRE(is<int8_t>(tokens[0].annotation));
		REQUIRE(get<int8_t>(tokens[0].annotation) == 0x0);
		REQUIRE(is<int8_t>(tokens[1].annotation));
		REQUIRE(get<int8_t>(tokens[1].annotation) == -0x1);
		REQUIRE(is<int8_t>(tokens[2].annotation));
		REQUIRE(get<int8_t>(tokens[2].annotation) == -0x7F);
		REQUIRE(is<int16_t>(tokens[3].annotation));
		REQUIRE(get<int16_t>(tokens[3].annotation) == -0x80);
		REQUIRE(is<int16_t>(tokens[4].annotation));
		REQUIRE(get<int16_t>(tokens[4].annotation) == -0x7FF);
		REQUIRE(is<int32_t>(tokens[5].annotation));
		REQUIRE(get<int32_t>(tokens[5].annotation) == -0x8000);
		REQUIRE(is<int32_t>(tokens[6].annotation));
		REQUIRE(get<int32_t>(tokens[6].annotation) == -0x7FFFFFFF);
		REQUIRE(is<int64_t>(tokens[7].annotation));
		REQUIRE(get<int64_t>(tokens[7].annotation) == -0x80000000_i64);
	}
	
	SECTION("diagnoses invalid literals") {
		{
			auto res = lexerHelper.parse("push 0xFFFFFFFFFFFFFFFF1");
			REQUIRE(res.numErrors >= 1);
			CHECK(res.reports[0].diagnosis.getCode() == DiagCode::InvalidNumericLiteral);
		}
		{
			auto res = lexerHelper.parse("push -0x8000000000000000");
			REQUIRE(res.numErrors >= 1);
			CHECK(res.reports[0].diagnosis.getCode() == DiagCode::InvalidNumericLiteral);
		}
	}
	
	SECTION("parses floating-point literals") {
		auto& tokens = lexerHelper.parseExpect("3.0 3.14 3.1415926535897932384626433832795", {
			TokenType::Numeric, TokenType::Numeric
		});
		
		CHECK(is<float>(tokens[0].annotation));
		CHECK(is<float>(tokens[1].annotation));
		CHECK(is<float>(tokens[2].annotation));
	}
}

TEST_CASE("Parser parses labels", "[Parser]") {
	SECTION("Labels can be defined") {
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

	SECTION("Labels can be referenced") {
		auto res = helper.parse(".code\nlabel: jmp label");
		REQUIRE(checkResult(res));
		auto& tokens = *res.info.tokens;
		REQUIRE(tokens.size() == 5);
		REQUIRE(is<const Label*>(tokens[3].annotation));
		auto ref = get<const Label*>(tokens[3].annotation);
		REQUIRE(ref == res.info.labels[0].get());
		REQUIRE(ref == get<const Label*>(tokens[1].annotation));
	}

	SECTION("Labels can be defined after referencing") {
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

TEST_CASE("Parser parses keywords", "[Parser]") {
	SECTION("global keyword") {
		auto res = helper.parse("global main\n.code\nmain:");
		REQUIRE(checkResult(res));
		auto& tokens = *res.info.tokens;
		REQUIRE(tokens.size() == 5);
		REQUIRE(tokens[0].type == TokenType::Keyword);
		REQUIRE(is<Keyword::Type>(tokens[0].annotation));
		CHECK(get<Keyword::Type>(tokens[0].annotation) == Keyword::Global);
		REQUIRE(tokens[1].type == TokenType::Label);
		REQUIRE(is<const Label*>(tokens[1].annotation));
		auto ref = get<const Label*>(tokens[1].annotation);
		REQUIRE(is<const Label*>(tokens[3].annotation));
		REQUIRE(ref == get<const Label*>(tokens[3].annotation));
		CHECK(ref->name == "main");
	}
}

TEST_CASE("Parser resolves mnemonics", "[Parser]") {
	SECTION("pushb") {
		auto annotation = parseAnnotation("push 0xFF", 0);
		REQUIRE(is<Instruction::Type>(annotation));
		CHECK(get<Instruction::Type>(annotation) == Instruction::PUSHB);
	}
	
	SECTION("pushw") {
		auto annotation = parseAnnotation("push 0xFFFF", 0);
		REQUIRE(is<Instruction::Type>(annotation));
		CHECK(get<Instruction::Type>(annotation) == Instruction::PUSHW);
	}
	
	SECTION("pushd") {
		auto annotation = parseAnnotation("push 0xFFFFFFFF", 0);
		REQUIRE(is<Instruction::Type>(annotation));
		CHECK(get<Instruction::Type>(annotation) == Instruction::PUSHD);
	}
	
	SECTION("pushq") {
		auto annotation = parseAnnotation("push 0xFFFFFFFFFF", 0);
		REQUIRE(is<Instruction::Type>(annotation));
		CHECK(get<Instruction::Type>(annotation) == Instruction::PUSHQ);
	}
}

TEST_CASE("Parser parses data segment definitions", "[Parser]") {
	SECTION("Byte data value") {
		auto res = helper.parse(".data\nBYTE_VALUE: DB 0xFF");
		auto& tokens = *res.info.tokens;
		REQUIRE(checkResult(res));
		REQUIRE(tokens.size() == 5);
		CHECK(tokens[0].type == TokenType::Segment);
		CHECK(tokens[1].type == TokenType::Label);
		CHECK(tokens[2].type == TokenType::DataType);
		CHECK(tokens[3].type == TokenType::Numeric);
		CHECK(is<const Label*>(tokens[1].annotation));
		CHECK(get<const Label*>(tokens[1].annotation)->segment == Segment::Data);
		CHECK(is<DataType::Type>(tokens[2].annotation));
		CHECK(get<DataType::Type>(tokens[2].annotation) == DataType::DB);
		CHECK(is<uint8>(tokens[3].annotation));
		CHECK(get<uint8>(tokens[3].annotation) == 0xFF);
	}
}

TEST_CASE("Error unexpected lexeme", "[Error Handling]") {
	auto res = lexerHelper.parse("`123");
	REQUIRE(res.hadFatal);
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::UnexpectedLexeme);
}

TEST_CASE("Error expected EndOfLine after segment", "[Error Handling]") {
	auto res = helper.parse(".code 1");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::ExpectedToken);
	CHECK(res.reports[0].diagnosis.get<DiagCode::ExpectedToken>().given == TokenType::Numeric);
	auto& diagnosis = res.reports[0].diagnosis.get<DiagCode::ExpectedToken>();
	CHECK(get<TokenType>(diagnosis.expected) == TokenType::EndOfLine);
}

TEST_CASE("Error unexpected separator", "[Error Handling]") {
	auto res = lexerHelper.parse(":");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::UnexpectedSeparator);
}

TEST_CASE("Error unexpected segment after tokens", "[Error Handling]") {
	auto res = lexerHelper.parse("nop .code");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::UnexpectedSegmentAfterTokens);
}

TEST_CASE("Error unexpected label after tokens", "[Error Handling]") {
	// parser picks up 'label' as an identifier (possible label pointer) and then errors on the ':' - this is ok?
	auto res = lexerHelper.parse(".code\nnop label:");
	REQUIRE(res.numErrors >= 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::UnexpectedLabelAfterTokens);
}

TEST_CASE("Error invalid identifier", "[Error Handling]") {
	auto res = helper.parse("blahdyblahbloo");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::InvalidIdentifier);
}

TEST_CASE("Error instruction passed as operand", "[Error Handling]") {
	auto res = lexerHelper.parse("nop nop");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::UnexpectedOperand);
	CHECK(res.reports[0].diagnosis.getMessage() == "unexpected instruction encountered, use ',' to separate multiple instructions on one line"s);
}

TEST_CASE("Error instruction missing operand", "[Error Handling]") {
	auto res = lexerHelper.parse("pushb");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::MissingOperand);
}

TEST_CASE("Error instruction invalid operand type", "[Error Handling]") {
	auto res = lexerHelper.parse("pushb \"str\"");
	REQUIRE(res.numErrors == 1);
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::InvalidOperandType);
}

TEST_CASE("Error undefined label", "[Error Handling]") {
	auto res = helper.parse(".code\njmp label");
	REQUIRE(res.numErrors >= 1);
	auto& tokens = *res.info.tokens;
	REQUIRE(tokens.size() >= 3);
	REQUIRE(tokens[2].type == TokenType::Label);
	REQUIRE(is<string>(tokens[2].annotation));
	CHECK(get<string>(tokens[2].annotation) == "label");
	REQUIRE(res.reports[0].diagnosis.getCode() == DiagCode::UnresolvedLabelReference);
}
