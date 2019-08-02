#pragma once
#include <CLASM/Common.h>
#include <CLASM/Assembly.h>
#include <CLASM/Token.h>
#include <CLASM/Label.h>

namespace CLARA::CLASM {
	using TokenAndString = pair<TokenType, string>;
	using AnyOfExpect = variant<TokenType, TokenAndString>;
	using AnyOf = vector<AnyOfExpect>;
	using Expected = variant<TokenType, TokenAndString, AnyOf>;

	enum class DiagCode : uint32_t {
		Unknown = 0,
		// Internal errors
		UnexpectedTokenBeganLine = 1000,       // a token sequence was parsed as a line when the type of the first token cannot be handled by the parser
		// Fatal errors
		UnexpectedLexeme = 2000,               // the lexer encountered a character or sequence of characters that were unexpected at that point
		ExpectedToken = 2001,                  // the parser encountered a token of the wrong type while expecting another one, or one of many
		UnexpectedToken = 2002,                // the parser is totally incapable of doing anything with this token here
		UnexpectedSeparator = 2003,            // a separator occurred in an invalid location
		UnexpectedSegmentAfterTokens = 2004,   // segment was not the first token on a line
		UnexpectedLabelAfterTokens = 2005,     // label was not the first token on a line
		UnexpectedOperand = 2006,              // more tokens provided than an instruction allows operands for
		InvalidIdentifier = 2010,              // identifier token could not be resolved to anything
		InvalidSegment = 2011,                 // segment token name is unrecognized
		InvalidOperandType = 2012,             // the wrong type of literal was passed for an operand
		InvalidMnemonicOperands = 2013,        // no instruction for the mnemonic has a matching series of operands
		MissingOperand = 2014,                 // another operand was required but the parser ran out of tokens
		LiteralValueSizeOverflow = 2015,       // a literal value was supplied that exceeds the bit size of the accepted operand
		InvalidEscapeSequence = 2016,          // literal string contained an invalid escape sequence, e.g. \z
		InvalidHexEscapeSequence = 2017,       // literal string contained an invalid hex escape sequence, e.g. \xGG
		LabelRedefinition = 2018,              // label encountered with a name that was already declared
		UnresolvedLabelReference = 2019,       // label reference with the label never defined
	};

	template<DiagCode TCode>
	struct DiagCodeTag {};

	class Diagnosis;

	template<DiagCode Code>
	struct Diagnostic;

	template<> struct Diagnostic<DiagCode::Unknown> {
		constexpr static auto name = "(internal error) unknown error"sv;
	};

	class Diagnosis {
		template <typename T, typename = void>
		struct has_format : std::false_type {};
		template <typename T>
		struct has_format<T, std::void_t<decltype(std::declval<T>().formatMessage())>> : std::true_type {};

	public:
		Diagnosis() : m_name(Diagnostic<DiagCode::Unknown>::name)
		{}

		template<DiagCode Code, typename... TArgs, std::enable_if_t<!has_format<Diagnostic<Code>>::value, int> = 0>
		Diagnosis(DiagCodeTag<Code>, TArgs&& ... args) noexcept :
			m_code(Code),
			m_name(Diagnostic<Code>::name),
			m_data(Diagnostic<Code>{forward<TArgs>(args)...})
		{}

		template<DiagCode Code, typename... TArgs, std::enable_if_t<has_format<Diagnostic<Code>>::value, int> = 0>
		Diagnosis(DiagCodeTag<Code>, TArgs&&... args) noexcept :
			m_code(Code),
			m_name(Diagnostic<Code>::name),
			m_data(Diagnostic<Code>{forward<TArgs>(args)...})
		{
			m_message = std::any_cast<Diagnostic<Code>&>(m_data).formatMessage();
		}

		template<DiagCode Code>
		auto& get()
		{
			return std::any_cast<Diagnostic<Code>&>(m_data);
		}

		template<DiagCode Code>
		auto& get() const
		{
			return std::any_cast<const Diagnostic<Code>&>(m_data);
		}

		constexpr auto getName() const noexcept
		{
			return m_name;
		}

		constexpr auto getCode() const noexcept
		{
			return m_code;
		}

		constexpr auto getCodeInt() const noexcept
		{
			return static_cast<std::underlying_type_t<DiagCode>>(m_code);
		}

		constexpr auto& getMessage() const noexcept
		{
			return m_message;
		}

		constexpr auto is(DiagCode code) const noexcept
		{
			return m_code == code;
		}

	private:
		DiagCode m_code = DiagCode::Unknown;
		string_view m_name;
		any m_data;
		string m_message;
	};

	template<DiagCode Code, typename... TArgs>
	auto diagnose(TArgs&&... args)
	{
		return Diagnosis{DiagCodeTag<Code>{}, forward<TArgs>(args)...};
	}

	template<> struct Diagnostic<DiagCode::UnexpectedTokenBeganLine> {
		constexpr static auto name = "(internal error) unexpected token began the line"sv;

		TokenType type;

		auto formatMessage() const
		{
			return to_string(type) + " not expected at beginning of line";
		}
	};

	template<> struct Diagnostic<DiagCode::UnexpectedLexeme> {
		constexpr static auto name = "unexpected lexeme"sv;

		auto formatMessage() const
		{
			return "invalid sequence of characters";
		}
	};
	
	template<> struct Diagnostic<DiagCode::ExpectedToken> {
		constexpr static auto name = "unexpected token"sv;

		TokenType given;                                   // the type of token that was given
		Expected expected;                                 // the type of token expected

		auto formatMessage() const
		{
			return to_string(given) + " encountered when expecting "s + std::visit(visitor{
				[](TokenType type) {
					return to_string(type);
				},
				[](const pair<TokenType, string>& tokenAndString) {
					return "'" + tokenAndString.second + "'";
				},
				[](const AnyOf& tokenTypes) {
					if (tokenTypes.empty()) {
						return "<error>"s;
					}

					auto expectToStr = [](const AnyOfExpect& expect) {
						return std::visit(visitor{
							[](TokenType type) {
								return to_string(type);
							},
							[](const pair<TokenType, string>& typeAndCode) {
								return "'"s + typeAndCode.second + "'"s;
							},
						}, expect);
					};
					auto begin = tokenTypes.begin();

					return "one of: "s + std::accumulate(begin + 1, tokenTypes.end(), expectToStr(*begin), [&](const string& str, auto val) {
						return str + ", "s + expectToStr(val);
					});
				},
			}, expected);
		}
	};

	template<> struct Diagnostic<DiagCode::UnexpectedToken> {
		constexpr static auto name = "unexpected token"sv;

		TokenType given;

		auto formatMessage() const
		{
			return to_string(given) + " not expected here"s;
		}
	};

	template<> struct Diagnostic<DiagCode::UnexpectedSeparator> {
		constexpr static auto name = "unexpected separator"sv;
	};
	
	template<> struct Diagnostic<DiagCode::UnexpectedSegmentAfterTokens> {
		constexpr static auto name = "unexpected segment"sv;

		auto formatMessage() const
		{
			return "segment should be on its own line"s;
		}
	};

	template<> struct Diagnostic<DiagCode::UnexpectedLabelAfterTokens> {
		constexpr static auto name = "unexpected label"sv;

		auto formatMessage() const
		{
			return "label should be the first token of a line"s;
		}
	};

	template<> struct Diagnostic<DiagCode::InvalidIdentifier> {
		constexpr static auto name = "invalid identifier";

		auto formatMessage() const
		{
			return "no definition found for this identifier (is a label missing?)"s;
		}
	};

	template<> struct Diagnostic<DiagCode::InvalidSegment> {
		constexpr static auto name = "invalid segment";

		auto formatMessage() const
		{
			return "segment not recognised"s;
		}
	};

	template<> struct Diagnostic<DiagCode::InvalidOperandType> {
		constexpr static auto name = "invalid operand type"sv;
		OperandType operand;                               // the type of operand that needed satisfying

		auto formatMessage() const
		{
			return "not a match for "s + to_string(operand);
		}
	};

	template<> struct Diagnostic<DiagCode::InvalidMnemonicOperands> {
		constexpr static auto name = "invalid operands for mnemonic"sv;
		Mnemonic::Type mnemonic;                           // the mnemonic that needed resolving

		auto formatMessage() const
		{
			return "given operands could not resolve this mnemonic to any instruction"s;
		}
	};

	template<> struct Diagnostic<DiagCode::MissingOperand> {
		constexpr static auto name = "too few operands"sv;
		OperandType operand;                               // type of operand that was missing

		auto formatMessage() const
		{
			return "missing an operand of type "s + to_string(operand);
		}
	};

	template<> struct Diagnostic<DiagCode::LiteralValueSizeOverflow> {
		constexpr static auto name = "value exceeds operand size"sv;
		OperandType operand;                               // the type of operand that needed satisfying

		auto formatMessage() const
		{
			return "value exceeds allowed size of "s + to_string(operand);
		}
	};

	template<> struct Diagnostic<DiagCode::InvalidHexEscapeSequence> {
		constexpr static auto name = "invalid hex escape sequence"sv;
		
		enum Problem {
			Unknown,
			NoHexChars,
			UnevenNumberOfChars,
			OutOfRange
		};

		Problem problem;

		auto formatMessage() const
		{
			switch (problem) {
			case Problem::NoHexChars:
				return "\\x must be followed by at least 1 hex number"s;
			case Problem::UnevenNumberOfChars:
				return "hex bytes must be represented in pairs (e.g. \x01 not \x1)"s;
			case Problem::OutOfRange:
				return "too many hex bytes - out of 32bit range"s;
			case Problem::Unknown: break;
			}
			return "invalid hex escape sequence"s;
		}
	};

	template<> struct Diagnostic<DiagCode::UnexpectedOperand> {
		constexpr static auto name = "unexpected operand";

		optional<TokenType> encountered;
		uint numExpected;
		uint numGiven;

		auto formatMessage() const
		{
			if (encountered) {
				if (*encountered == TokenType::Instruction || *encountered == TokenType::Mnemonic) {
					return "unexpected instruction encountered, use ',' to separate multiple instructions on one line"s;
				}
			}

			if (!numExpected) {
				return fmt::format("instruction takes no operands, {} provided", numGiven);
			}

			if (numGiven > 1) {
				return fmt::format("expected {} {}, found {}", numExpected, numExpected == 1 ? "operand" : "operands", numGiven);
			}

			return "unexpected additional operand"s;
		}
	};

	template<> struct Diagnostic<DiagCode::LabelRedefinition> {
		constexpr static auto name = "label redefinition";

		const Label& original;

		auto formatMessage() const
		{
			return "label already defined on line "s + to_string(original.definition.getLineNumber());
		}
	};

	template<> struct Diagnostic<DiagCode::UnresolvedLabelReference> {
		constexpr static auto name = "unresolved label reference";

		auto formatMessage() const
		{
			return "no label with this name is defined"s;
		}
	};
}