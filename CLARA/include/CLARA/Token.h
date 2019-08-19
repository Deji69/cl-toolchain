#pragma once
#include <CLARA/Common.h>
#include <CLARA/Assembly.h>
#include <CLARA/Source.h>

namespace CLARA::CLASM {

struct Label;

enum class TokenType {
	EOL,
	WhiteSpace,
	Separator,
	Directive,
	Segment,
	String,
	Identifier, // pre-parsed form of one of:
		Keyword,
		Label,
		Mnemonic,
		Instruction,
	Numeric, // generic, parsed form of one of:
		HexLiteral,
		IntegerLiteral,
		FloatLiteral,
};

using TokenAnnotation = variant<
	monostate,
	uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t,
	float, double,
	string,
	const Label*,
	Keyword::Type,
	Segment::Type,
	Mnemonic::Type,
	Instruction::Type
>;

struct Token : public Source::Token {
	TokenType type = TokenType::EOL;
	TokenAnnotation annotation;

	Token() = default;
	Token(const Source* source, TokenType type, size_t offset = 0, string_view text = string_view{});
	Token(const Source* source, TokenType type, size_t offset, size_t length);

	auto operator=(const Token&)->Token& = default;

	auto is(TokenType type) const->bool;
	auto is(TokenType type, string_view value) const->bool;
	auto getText() const->string_view;
	auto getLineInfo() const->const Source::LineInfo&;
	auto getLineNumber() const->size_t;
	auto getAssemblySize() const->size_t;
};

}

template<>
inline auto CLARA::to_string(CLASM::TokenType type)
{
	switch (type) {
	case CLASM::TokenType::EOL:
		return "end-of-line"s;
	case CLASM::TokenType::WhiteSpace:
		return "white space"s;
	case CLASM::TokenType::Separator:
		return "separator"s;
	case CLASM::TokenType::Keyword:
		return "keyword"s;
	case CLASM::TokenType::Directive:
		return "directive"s;
	case CLASM::TokenType::Segment:
		return "segment"s;
	case CLASM::TokenType::Label:
		return "label"s;
	case CLASM::TokenType::Identifier:
		return "identifier"s;
	case CLASM::TokenType::Mnemonic:
		return "mnemonic"s;
	case CLASM::TokenType::Instruction:
		return "instruction"s;
	case CLASM::TokenType::Numeric:
		return "numeric literal"s;
	case CLASM::TokenType::HexLiteral:
	case CLASM::TokenType::IntegerLiteral:
		return "integer literal"s;
	case CLASM::TokenType::FloatLiteral:
		return "floating-point literal"s;
	case CLASM::TokenType::String:
		return "string literal"s;
	}

	throw std::invalid_argument("Unhandled token type name");
}
