#include <CLASM/pch.h>
#include <CLASM/Token.h>

using namespace CLARA;
using namespace CLARA::CLASM;

auto getAnnotationSize(const TokenAnnotation& annotation)
{
	if (is<monostate>(annotation))
		return 0;
	if (is<int8_t>(annotation) || is<uint8_t>(annotation))
		return 1;
	if (is<int16_t>(annotation) || is<uint16_t>(annotation))
		return 2;
	if (is<int32_t>(annotation) || is<uint32_t>(annotation) || is<float>(annotation))
		return 4;
	if (is<int64_t>(annotation) || is<uint64_t>(annotation) || is<double>(annotation))
		return 8;
	if (is<Instruction::Type>(annotation))
		return 1;
	return 0;
}

Token::Token(TokenType type, size_t offset, string_view text) : Source::Token(offset, text),
	type(type)
{ }

auto Token::is(TokenType type_) const->bool
{
	return type == type_;
}

auto Token::is(TokenType type_, string_view value) const->bool
{
	return is(type_) && text == value;
}

auto Token::getText() const->string_view
{
	return text;
}

auto Token::getAssemblySize() const->size_t
{
	return getAnnotationSize(annotation);
}
