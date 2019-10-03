#include <CLARA/Token.h>
#include <CLARA/pch.h>

using namespace CLARA;

namespace CLARA::CLASM {

auto getAnnotationSize(const TokenAnnotation& annotation) {
	if (is<monostate>(annotation)) return 0;
	if (is<int8_t>(annotation) || is<uint8_t>(annotation)) return 1;
	if (is<int16_t>(annotation) || is<uint16_t>(annotation)) return 2;
	if (is<int32_t>(annotation) || is<uint32_t>(annotation) || is<float>(annotation)) return 4;
	if (is<int64_t>(annotation) || is<uint64_t>(annotation) || is<double>(annotation)) return 8;
	if (is<Instruction::Type>(annotation)) return 1;
	return 0;
}

auto getAnnotationTokenType(const TokenAnnotation& annotation)->TokenType {
	return std::visit([](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_arithmetic_v<T>)
			return TokenType::Numeric;
		else if constexpr (std::is_same_v<T, string>)
			return TokenType::String;
		else if constexpr (std::is_same_v<T, const Label*>)
			return TokenType::Label;
		else if constexpr (std::is_same_v<T, LabelRef>)
			return TokenType::LabelRef;
		else if constexpr (std::is_same_v<T, Keyword::Type>)
			return TokenType::Keyword;
		else if constexpr (std::is_same_v<T, Segment::Type>)
			return TokenType::Segment;
		else if constexpr (std::is_same_v<T, Mnemonic::Type>)
			return TokenType::Mnemonic;
		else if constexpr (std::is_same_v<T, Instruction::Type>)
			return TokenType::Instruction;
		else if constexpr (std::is_same_v<T, DataType::Type>)
			return TokenType::DataType;
		return TokenType::None;
	}, annotation);
	return TokenType::None;
}

Token::Token(TokenType type, TokenAnnotation annotation) :
	type(type), annotation(annotation)
{}

Token::Token(const Source* source, TokenType type, size_t offset, string_view text) :
	Source::Token(source, offset, text), type(type)
{}

Token::Token(const Source* source, TokenType type, size_t offset, size_t length) :
	Source::Token(source, offset, length), type(type)
{}

auto Token::is(TokenType type_) const -> bool { return type == type_; }

auto Token::is(TokenType type_, string_view value) const -> bool {
	return is(type_) && text == value;
}

auto Token::getLineInfo() const -> const Source::LineInfo& {
	auto idx = source->getLineIndexByOffset(static_cast<uint>(offset));
	return source->getLineInfo(idx);
}

auto Token::getLineNumber() const -> size_t {
	if (!source) return 0;
	return source->getLineIndexByOffset(static_cast<uint>(offset)) + 1;
}

auto Token::getText() const -> string_view { return text; }

auto Token::getAssemblySize() const -> size_t { return getAnnotationSize(annotation); }

}