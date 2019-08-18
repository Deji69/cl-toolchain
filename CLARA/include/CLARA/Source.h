#pragma once
#include <CLARA/Common.h>

namespace CLARA::CLASM {

class Source {
public:
	/// A portion of the source code.
	struct Token {
		size_t offset = 0;
		string_view text;
		const Source* source = nullptr;

		Token() = default;
		Token(Token&&) = default;
		Token(const Token&) = default;
		Token(const Token& first, const Token& last);
		Token(const Source*, size_t offset, string_view text);
		Token(const Source*, size_t offset, size_t length);
		
		auto operator=(Token&&)->Token& = default;
		auto operator=(const Token&)->Token& = default;
	};

	/// Information about a line of source code.
	struct LineInfo {
		uint number = 1;				//< 1-based line number
		uint offset = 0;				//< line offset in bytes
		uint length = 0;				//< line length in bytes
		uint charOffset = 0;			//< line offset in characters
		uint charLength = 0;			//< line length in characters
	};

public:
	/**
	 * Construct source object.
	 *
	 * @param  name The logical name of the source code.
	 * @param  code The source code string.
	 */
	Source(string name, string code);

	/**
	 * Get the logical source name.
	 *
	 * @return The logical source name.
	 */
	auto getName() const->const string&;

	/**
	 * Get the code string.
	 *
	 * @return The code string.
	 */
	auto getCode() const->const string&;

	/**
	 * Get number of code lines.
	 *
	 * @return The number of lines of code.
	 */
	auto getNumLines() const->uint;

	/**
	 * Get line informations.
	 *
	 * @return The vector of line informations.
	 */
	auto getLineInfos() const->const vector<LineInfo>&;

	/**
	 * Get line information.
	 *
	 * @param  index The 0-based index to get the line info of.
	 * @return The line information.
	 */
	auto getLineInfo(uint index) const->const LineInfo&;

	/**
	 * Get the line index at an offset.
	 *
	 * @param  offset The offset to get the line index of.
	 * @return The 0-based index of the line.
	 */
	auto getLineIndexByOffset(uint offset) const->uint;

	/**
	 * Get the line column number of a code offset.
	 *
	 * @param  offset The offset to get the column number of.
	 * @return The line column number.
	 */
	auto getColumnByOffset(uint offset) const->uint;

	/**
	 * Get a substring of the code.
	 *
	 * @param  from The starting position of the substring.
	 * @param  size The length of the substring.
	 * @return The substring.
	 */
	auto getText(size_t from, size_t size) const->string_view;

	/**
	 * Get a token for the code.
	 *
	 * Get a token from `from` up to the first whitespace character or the end of the code.
	 *
	 * @param  from The starting position of the token.
	 * @return The token substring.
	 * @throws std::range_error if the requested range is invalid.
	 */
	auto getToken(size_t from) const->Token;

	/**
	 * Get a token for the code.
	 *
	 * Get a token from `from` with at most the number of bytes specified by `size`.
	 *
	 * @param  from The starting position of the token.
	 * @param  size The size of the token in bytes.
	 * @return The token substring.
	 * @throws std::range_error if the requested range is invalid.
	 */
	auto getToken(size_t from, size_t size) const->Token;

private:
	const string m_name;
	const string m_code;
	vector<LineInfo> m_lineInfos;
	map<uint, uint> m_offsetLines;
};

}