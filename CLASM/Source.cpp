#include <CLASM/pch.h>
#include <CLASM/Source.h>

using namespace CLARA;
using namespace CLARA::CLASM;

Source::Token::Token(const Token& first, const Token& last) :
	source(first.source), offset(first.offset),
	text(string_view{first.text.data(), last.offset - first.offset + last.text.size()})
{
	assert(first.source == last.source);
	assert(last.text.data() >= first.text.data());
}

Source::Token::Token(const Source* source, size_t offset, string_view text) :
	source(source), offset(offset), text(text)
{
	assert(source != nullptr);
}

Source::Token::Token(const Source* source, size_t offset, size_t length) :
	source(source), offset(offset), text(source->getText(offset, length))
{
	assert(source != nullptr);
}

Source::Source(string name, string code) : m_name(name), m_code(code)
{
	LineInfo line;

	for (size_t i = 0, chars = 0; i < code.size(); ++i, ++chars) {
		if (code[i] == '\n') {
			m_offsetLines.emplace(line.offset, m_lineInfos.size());
			m_lineInfos.emplace_back(line);
			line = LineInfo{line.number + 1, i + 1, 0, chars};
			continue;
		}

		++line.length;
		++line.charLength;

		if ((code[i] & 0xF0) == 0xF0) {
			i += 3;
			line.length += 3;
		}
		else if ((code[i] & 0xE0) == 0xE0) {
			i += 2;
			line.length += 2;
		}
		else if ((code[i] & 0xC0) == 0xC0) {
			i += 1;
			line.length += 1;
		}
		else continue;

		if (i >= code.size()) {
			throw std::runtime_error{"Invalid UTF-8 string in code"};
		}
	}

	m_offsetLines.emplace(line.offset, m_lineInfos.size());
	m_lineInfos.push_back(line);
}

auto Source::getName() const->const string&
{
	return m_name;
}

auto Source::getCode() const->const string&
{
	return m_code;
}

auto Source::getNumLines() const->uint
{
	return m_lineInfos.size();
}

auto Source::getLineInfos() const->const vector<LineInfo>&
{
	return m_lineInfos;
}

auto Source::getLineInfo(uint index) const->const LineInfo&
{
	return m_lineInfos[index];
} 

auto Source::getLineIndexByOffset(uint offset) const->uint
{
	auto it = m_offsetLines.upper_bound(offset);
	if (it != m_offsetLines.begin()) --it;
	return it != m_offsetLines.end() ? it->second : 0;
}

auto Source::getColumnByOffset(uint offset) const->uint
{
	auto& line = getLineInfo(getLineIndexByOffset(offset));
	auto numBytes = offset - line.offset;
	
	if ((line.offset + numBytes) > m_code.size()) {
		return line.charLength;
	}

	auto token = this->getText(line.offset, numBytes);
	return std::accumulate(token.begin(), token.end(), 1, [](int n, char c) {
		return n + int{(c & 0xC0) != 0x80 ? 1 : 0};
	});
}

auto Source::getText(size_t from, size_t size) const->string_view
{
	if ((from + size) > m_code.size()) {
		throw std::range_error{"Invalid source code range requested"};
	}
	return string_view{m_code.c_str() + from, size};
}

auto Source::getToken(size_t from) const->Token
{
	if (from >= m_code.size()) {
		throw std::out_of_range("offset not in range of source");
	}

	auto end = m_code.c_str() + m_code.size();
	auto begin = std::find_if(m_code.c_str() + from, end, std::not_fn(::isspace));
	auto it = std::find_if(begin, end, ::isspace);

	return Token{this, static_cast<size_t>(begin - m_code.c_str()), static_cast<size_t>(it - begin)};
}

auto Source::getToken(size_t from, size_t size) const->Token
{
	if (from >= m_code.size()) {
		throw std::out_of_range("offset not in range of source");
	}

	if ((from + size) >= m_code.size()) {
		size = m_code.size() - from;
	}

	return Token{this, from, size};
}