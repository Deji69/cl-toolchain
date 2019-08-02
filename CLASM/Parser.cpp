#include <CLASM/pch.h>
#include <CLASM/Parser.h>

using namespace CLARA::CLASM;

namespace CLARA::CLASM::Parser {

using TokenVec = vector<Token>;

struct LexRule {
	TokenType type;
	function<size_t(const char*, size_t)> func;
};

struct LexResult {
	TokenType type;
	size_t length;

	LexResult(TokenType type, size_t length) : type(type), length(length)
	{ }
};

struct Continue {
	variant<Token, TokenVec> token;

	Continue() : token(TokenVec{})
	{
		get<TokenVec>(token).reserve(16);
	}

	Continue(Token token) : token(token)
	{ }

	Continue(Token&& token) : token(move(token))
	{ }

	auto add(const variant<Token, TokenVec>& var)
	{
		toVector();

		auto& vec = get<TokenVec>(token);

		if (is<Token>(var)) {
			vec.emplace_back(get<Token>(var));
		}
		else {
			auto& contVec = get<TokenVec>(var);
			std::copy(contVec.begin(), contVec.end(), vec.begin());
		}
	}

	auto toVector()->TokenVec&
	{
		if (!is<TokenVec>(token)) {
			auto tok = move(get<Token>(token));
			token.emplace<TokenVec>().emplace_back(move(tok));
		}
		return get<TokenVec>(token);
	}
};

struct Finish {
	optional<Token> token;
	optional<Expected> expected;
	vector<Report> reports;

	Finish() = default;

	Finish(Token&& token) : token(token)
	{ }

	Finish(const Token& token) : token(token)
	{ }

	auto& merge(const Finish& other)
	{
		reports.insert(reports.end(), other.reports.begin(), other.reports.end());
		return *this;
	}

	auto& expect(TokenType type)
	{
		expected.emplace(type);
		return *this;
	}

	auto& expect(TokenType type, string value)
	{
		expected.emplace(make_pair(type, value));
		return *this;
	}

	auto& expect(AnyOf types)
	{
		expected.emplace(types);
		return *this;
	}

	template<typename... TArgs>
	auto& report(TArgs&&... args)
	{
		reports.emplace_back(forward<TArgs>(args)...);
		return *this;
	}

	template<typename... TArgs>
	auto& warn(TArgs&&... args)
	{
		reports.emplace_back(ReportType::Warning, forward<TArgs>(args)...);
		return *this;
	}

	template<typename... TArgs>
	auto& error(TArgs&&... args)
	{
		reports.emplace_back(ReportType::Error, forward<TArgs>(args)...);
		return *this;
	}
};

struct Fatal {
	Report error;

	Fatal() = default;

	Fatal(Source::Token token) : error(ReportType::Fatal, token, diagnose<DiagCode::Unknown>())
	{ }

	Fatal(Source::Token token, Diagnosis&& info) : error(ReportType::Fatal, token, forward<Diagnosis>(info))
	{ }
};

struct Success {
	vector<Token> tokens;

	template<typename... TArgs>
	decltype(auto) addToken(TArgs&&... args)
	{
		return tokens.emplace_back(forward<TArgs>(args)...);
	}
};

struct Error {
	Source::Token token;
	Diagnosis info;
};

using ParseState = variant<Finish, Continue, Fatal>;
using ParseResult = variant<Success, Error>;

struct State {
	ParseInfo& info;
	Segment::Type segment = Segment::None;
};

auto parseIdentifier(const ParseState& state, Token&& token)->ParseState
{
	auto id = string{token.text};
	
	if (auto kw = Keyword::fromName(id)) {
		token.type = TokenType::Keyword;
		token.annotation.emplace<Keyword::Type>(*kw);
		return Continue{token};
	}

	if (auto mnemonic = Mnemonic::fromName(id)) {
		token.type = TokenType::Mnemonic;
		token.annotation.emplace<Mnemonic::Type>(*mnemonic);
		return Continue{token};
	}

	if (auto insn = Instruction::fromName(id)) {
		token.type = TokenType::Instruction;
		token.annotation.emplace<Instruction::Type>(*insn);
		return Continue{token};
	}

	if (is<Continue>(state)) {
		return Continue(token);
	}

	return Finish(token);
}

auto parseSegment(const ParseState& state, Token&& token)->ParseState
{
	if (is<Continue>(state)) {
		return Finish().error(token, diagnose<DiagCode::UnexpectedSegmentAfterTokens>());
	}

	auto id = token.text.substr(1);

	if (auto segment = Segment::fromName(id)) {
		token.annotation = *segment;
		return Finish(token);
	}

	return Finish().error(token, diagnose<DiagCode::InvalidSegment>());
}

auto parseString(const ParseState&, Token&& token)->ParseState
{
	auto result = Finish{};
	auto str = string{};
	str.reserve(token.text.size());

	// get just the stuff between the quotes
	auto text = token.text.substr(1, token.text.size() - 2);
	auto l = size_t{0};

	for (size_t i = text.find_first_of('\\'); i != text.npos; l = i, i = text.find_first_of('\\', i)) {
		auto sv = text.substr(i);
	
		// there theoretically can't be a single backslash as the last character of the string as then we should have
		// never been able to lex the closing quote
		assert(sv.size() > 1);
		if (sv.size() <= 1)
			break;

		str += text.substr(l, i - l);
		i += 2;

		switch (sv[1]) {
		case '\\': str += '\\'; break;
		case 'r': str += '\r'; break;
		case 'n': str += '\n'; break;
		case 't': str += '\t'; break;
		case '"': str += '"'; break;
		case 'x':
			{
				sv = sv.substr(0, std::distance(sv.begin(), std::find_if_not(sv.begin() + 2, sv.end(), [](auto ch) {
					return std::isxdigit(ch);
				})));

				auto seq = sv.substr(2);
				auto value = uint32_t{};
				auto res = std::from_chars(seq.data(), seq.data() + seq.size(), value, 16);
				auto dist = std::distance(seq.data(), res.ptr);
				i += dist;

				if (i < text.size() && text[i] == '\\')
					++i;
				
				if (res.ec != std::errc{}) {
					if (!dist) {
						result.error(
							Source::Token(token.source, token.offset + std::distance(token.text.data(), sv.data()), sv),
							diagnose<DiagCode::InvalidHexEscapeSequence>(
								Diagnostic<DiagCode::InvalidHexEscapeSequence>::Problem::NoHexChars
							)
						);
					}
					else {
						// get what we can from the sequence even though we can't represent the whole thing
						res = std::from_chars(seq.data(), res.ptr, value, 16);

						seq = seq.substr(0, dist);

						result.error(
							Source::Token(token.source, token.offset + std::distance(token.text.data(), sv.data()) + dist, seq),
							diagnose<DiagCode::InvalidHexEscapeSequence>(
								res.ec == std::errc::result_out_of_range
								? Diagnostic<DiagCode::InvalidHexEscapeSequence>::Problem::OutOfRange
								: Diagnostic<DiagCode::InvalidHexEscapeSequence>::Problem::Unknown
							)
						);
					}
				}

				if (!seq.empty()) {
					auto len = seq.size() / 2;

					if (!len || seq.size() % 2 != 0)
						++len;

					switch (len) {
					case 1: str.append(encodeBytesLE<decltype(value), 1>(value).data(), len); break;
					case 2: str.append(encodeBytesLE<decltype(value), 2>(value).data(), len); break;
					case 3: str.append(encodeBytesLE<decltype(value), 3>(value).data(), len); break;
					case 4: str.append(encodeBytesLE<decltype(value), 4>(value).data(), len); break;
					}
				}
			}
			break;
		}
	}

	if (l == 0) {
		str = text;
	}
	else {
		str += text.substr(l);
	}

	token.annotation = move(str);
	result.token = move(token);
	return result;
}

auto parseToken(const ParseState& state, Token&& token)->ParseState
{
	switch (token.type) {
	default: break;
	case TokenType::EOL: return Finish();
	case TokenType::Identifier: return parseIdentifier(state, forward<Token>(token));
	case TokenType::Segment: return parseSegment(state, forward<Token>(token));
	case TokenType::String: return parseString(state, forward<Token>(token));
	case TokenType::Label:
	case TokenType::Numeric:
		return Finish(forward<Token>(token));
	case TokenType::Separator:
		if (token.text == ",") {
			return Finish();
		}
		return Finish().error(forward<Token>(token), diagnose<DiagCode::UnexpectedSeparator>());
	}
	return Fatal(forward<Token>(token), diagnose<DiagCode::UnexpectedToken>(token.type));
}

auto checkOperandType(OperandType type, const Token& token)->ParseResult
{
	switch (type) {
	case OperandType::IMM8:
		if (is<int16_t>(token.annotation) || is<uint16_t>(token.annotation))
			return Error{token, diagnose<DiagCode::LiteralValueSizeOverflow>(type)};
	case OperandType::IMM16:
		if (is<int32_t>(token.annotation) || is<uint32_t>(token.annotation))
			return Error{token, diagnose<DiagCode::LiteralValueSizeOverflow>(type)};
	case OperandType::IMM32:
		if (is<int64_t>(token.annotation) || is<uint64_t>(token.annotation))
			return Error{token, diagnose<DiagCode::LiteralValueSizeOverflow>(type)};
	case OperandType::IMM64:
		if (token.type != TokenType::Numeric)
			return Error{token, diagnose<DiagCode::InvalidOperandType>(type)};
		break;

	case OperandType::REL32:

	case OperandType::V16:
	case OperandType::V32:
	case OperandType::LV8:
	case OperandType::LV16:
	case OperandType::LV32:
		return Error{};

	case OperandType::S32:
		if (token.type == TokenType::String)
			break;
		return Error{token, diagnose<DiagCode::InvalidOperandType>(type)};
	}

	return Success{};
}

auto parseInstructionLine(TokenVec& tokens)->ParseResult
{
	auto parseLine = [&](Instruction::Type instruction)->ParseResult {
		auto success = Success{};
		auto& operands = Instruction::getOperands(instruction);
		auto begin = std::begin(tokens) + 1;
		auto end = std::end(tokens);
		auto it = begin;

		success.tokens.reserve(static_cast<size_t>(std::abs(std::distance(it, end) + 1)));

		auto& insnToken = success.addToken(tokens[0].source, TokenType::Instruction, tokens[0].offset, tokens[0].text.size());
		insnToken.annotation = instruction;

		auto parseOperand = [&success](OperandType type, const Token& token)->ParseResult {
			auto res = checkOperandType(type, token);

			if (is<Error>(res)) {
				return get<Error>(res);
			}

			success.tokens.emplace_back(token);
			return Success{};
		};

		for (auto& operand : operands) {
			if (operand.variadic) {
				for (auto cur = it; it != end; ++cur) {
					for (auto type : operand.types) {
						if (cur == end) {
							auto last = std::prev(cur);
							return Error{Source::Token(*begin, *last), diagnose<DiagCode::MissingOperand>(type)};
						}

						auto result = parseOperand(type, *cur++);

						if (is<Error>(result)) {
							return get<Error>(result);
						}
					}
				}
			}
			else {
				for (auto type : operand.types) {
					if (it == end) {
						auto token = begin != end ? Source::Token(*begin, *std::prev(it)) : *std::begin(tokens);
						return Error{token, diagnose<DiagCode::MissingOperand>(type)};
					}

					auto result = parseOperand(type, *it++);

					if (is<Error>(result))
						return get<Error>(result);
				}
			}
		}

		if (it != end) {
			auto last = end - 1;
			auto token = it != last ? Source::Token(*it, *last) : Source::Token(*it);
			auto numExpected = static_cast<uint>(operands.size());
			auto numProvided = static_cast<uint>(std::distance(begin, end));
			return Error{token, diagnose<DiagCode::UnexpectedOperand>(it->type, numExpected, numProvided)};
		}

		return move(success);
	};

	if (tokens[0].type == TokenType::Mnemonic) {
		auto mnemonic = get<Mnemonic::Type>(tokens[0].annotation);

		for (auto& overload : Mnemonic::getOverloads(mnemonic)) {
			auto res = parseLine(overload.insn);

			if (auto success = get_if<Success>(&res)) {
				return *success;
			}
		}

		return Error{Source::Token(tokens.front(), tokens.back()), diagnose<DiagCode::InvalidMnemonicOperands>(mnemonic)};
	}
	return parseLine(get<Instruction::Type>(tokens[0].annotation));
}

auto parseLine(State& parser, Finish&& state)->ParseState
{
	if (!state.token) {
		return state;
	}

	auto token = &*state.token;

	switch (state.token->type) {
	default: break;

	case TokenType::Identifier:
		return Finish().error(parser.info.tokens->push(move(*token)), diagnose<DiagCode::InvalidIdentifier>());

	case TokenType::Segment:
		parser.segment = get<Segment::Type>(token->annotation);
		parser.info.tokens->push(move(*token));
		return Finish().expect(TokenType::EOL);

	case TokenType::Label:
		{
			auto name = state.token->text.substr(0, state.token->text.size() - 1);
			token = &parser.info.tokens->push(move(*token));

			auto res = parser.info.labels.emplace(Label{string(name), *token});

			if (!res.second) {
				return Finish().error(*token, diagnose<DiagCode::LabelRedefinition>(*res.first));
			}
		}
		return Finish();

	case TokenType::Instruction:
		parser.info.tokens->push(move(*token));
		return Finish().expect({TokenType::EOL, make_pair(TokenType::Separator, ","s)});
	}

	auto& addedToken = parser.info.tokens->push(move(*token));
	return Finish().error(addedToken,diagnose<DiagCode::UnexpectedToken>(addedToken.type));
}

auto parseLine(State& parser, Continue&& state)->ParseState
{
	auto& tokens = state.toVector();

	if (!tokens.empty()) {
		switch (tokens[0].type) {
		case TokenType::Mnemonic:
		case TokenType::Instruction:
			{
				auto res = parseInstructionLine(tokens);
				
				if (is<Error>(res)) {
					return Finish().error(tokens[0], move(get<Error>(res).info));
				}

				for (auto& token : get<Success>(res).tokens) {
					parser.info.tokens->push(token);
				}
			}
			return Finish().expect({TokenType::EOL, make_pair(TokenType::Separator, ","s)});

		case TokenType::Keyword:
			break;

		case TokenType::Label:
		case TokenType::Segment:
			throw ParseException("unexpected token in continue state");

		default:
			return Fatal(tokens[0], diagnose<DiagCode::UnexpectedTokenBeganLine>());
		}
	}
	return Finish();
}

auto lexComment(const char* str, size_t len)
{
	if (str[0] == ';') {
		auto it = std::find(str + 1, str + len, '\n');
		if (it < (str + len)) {
			return static_cast<size_t>(std::distance(str, it));
		}
	}
	return size_t{0};
}

auto lexWhitespace(const char* str, size_t len)
{
	auto it = std::find_if_not(str, str + len, [](char c) {
		return std::isspace(c);
	});
	return static_cast<size_t>(std::distance(str, it));
}

auto lexWhitespaceNoNewlines(const char* str, size_t len)
{
	auto it = std::find_if_not(str, str + len, [](char c) {
		return c != '\n' && std::isspace(c);
	});
	return static_cast<size_t>(std::distance(str, it));
}

auto lexNewline(const char* str, size_t len)
{
	auto it = std::find_if_not(str, str + len, [](char c) {
		return c == '\n';
	});
	return static_cast<size_t>(std::distance(str, it));
}

auto lexIdentifier(const char* str, size_t len)
{
	if (std::isalpha(str[0]) || str[0] == '_') {
		auto it = std::find_if_not(str + 1, str + len, [](char c) {
			return c == '_' || std::isalnum(c);
		});
		if (it != str) {
			return static_cast<size_t>(std::distance(str, it));
		}
	}
	return size_t{0};
}

auto lexSegment(const char* str, size_t len)
{
	if (str[0] == '.') {
		if (auto res = lexIdentifier(str + 1, len - 1)) {
			return static_cast<size_t>(res) + 1;
		}
	}
	return size_t{0};
}

auto lexLabel(const char* str, size_t len)
{
	if (auto res = lexIdentifier(str, len)) {
		if (str[res] == ':') {
			if (lexWhitespace(str + res + 1, len - res - 1)) {
				return res + 1;
			}
		}
	}
	return size_t{0};
}

auto lexNumeric(const char* str, size_t)
{
	static std::regex numberRegex{"^[+\\-]?(?:0|[1-9]\\d*)(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?\\b"};
	std::smatch sm;
	auto s = string{str};
	if (std::regex_search(s, sm, numberRegex)) {
		return static_cast<size_t>(sm[0].length());
	}
	return size_t{0};
}

auto lexString(const char* str, size_t len)
{
	if (str[0] == '"') {
		size_t end = 0;
		auto sv = string_view{str, len};

		for (auto pos = sv.find('"', 1); pos != sv.npos; pos = sv.find('"', pos + 1)) {
			auto rpos = pos - 1;
				
			if (rpos != 1) {
				while (sv[rpos] == '\\') {
					--rpos;
				}

				if (((pos - rpos - 1) % 2) != 0)
					continue;
			}

			end = pos + 1;
			break;
		}

		if (end)
			return end;
	}
	return size_t{0};
}

auto lexSeparator(const char* str, size_t)
{
	switch (*str) {
	case '=':
	case ':':
	case ',':
		return size_t{1};
	}
	return size_t{0};
}

auto lexOne(string_view sv, LexRule rule)->optional<LexResult>
{
	if (auto len = rule.func(sv.data(), sv.size()))
		return make_optional<LexResult>(rule.type, len);
	return nullopt;
}

auto lexOneOf(string_view sv, initializer_list<LexRule> rules)->optional<LexResult>
{
	for (auto& rule : rules) {
		if (auto res = lexOne(sv, rule)) {
			return res;
		}
	}
	return nullopt;
}

Report::Report(ReportType type, Source::Token token, Diagnosis&& diagnosis) :
	type(type), token(token), diagnosis(diagnosis)
{ }

auto Report::info(const Source::Token& token, Diagnosis&& diagnosis)->Report
{
	return Report(ReportType::Info, token, forward<Diagnosis>(diagnosis));
}

auto Report::warning(const Source::Token& token, Diagnosis&& diagnosis)->Report
{
	return Report(ReportType::Warning, token, forward<Diagnosis>(diagnosis));
}

auto Report::error(const Source::Token& token, Diagnosis&& diagnosis)->Report
{
	return Report(ReportType::Error, token, forward<Diagnosis>(diagnosis));
}

auto Report::fatal(const Source::Token& token, Diagnosis&& diagnosis)->Report
{
	return Report(ReportType::Fatal, token, forward<Diagnosis>(diagnosis));
}

const auto lexRules = initializer_list<LexRule>{
	{TokenType::EOL, lexNewline},
	{TokenType::WhiteSpace, lexComment},
	{TokenType::WhiteSpace, lexWhitespaceNoNewlines},
	{TokenType::Separator, lexSeparator},
	{TokenType::Segment, lexSegment},
	{TokenType::String, lexString},
	{TokenType::Numeric, lexNumeric},
	{TokenType::Label, lexLabel},
	{TokenType::Identifier, lexIdentifier},
};

auto getExpectedTokenError(const Expected& expect, const Token& token)
{
	return std::visit(visitor{
		[token](TokenType type)->optional<Report> {
			if (token.type != type) {
				return Report::error(token, diagnose<DiagCode::ExpectedToken>(token.type, type));
			}
			return nullopt;
		},
		[token](const pair<TokenType, string>& typeAndValue)->optional<Report> {
			if (token.type != typeAndValue.first || token.text != typeAndValue.second) {
				return Report::error(token, diagnose<DiagCode::ExpectedToken>(token.type, typeAndValue));
			}
			return nullopt;
		},
		[token](const AnyOf& types)->optional<Report> {
			if (!std::any_of(types.begin(), types.end(), [token](const AnyOfExpect& expected) {
				return std::visit(visitor{
					[&](TokenType type) { return token.type == type; },
					[&](const pair<TokenType, string>& typeAndStr) {
						return token.type == typeAndStr.first && token.getText() == typeAndStr.second;
					},
				}, expected);
			})) {
				auto ss = stringstream{};
				return Report::error(token, diagnose<DiagCode::ExpectedToken>(token.type, types));
			}
			return nullopt;
		}
	}, expect);
}

auto& addExpectationsForSegment(Finish& state, Segment::Type segment)
{
	switch (segment) {
	case Segment::None: state.expect({TokenType::EOL, TokenType::Identifier, TokenType::Segment}); break;
	case Segment::Code: state.expect({TokenType::EOL, TokenType::Identifier, TokenType::Label, TokenType::Segment}); break;
	case Segment::Data: state.expect({TokenType::EOL, TokenType::Label, TokenType::Segment}); break;
	}
	return state;
}

auto Parser::tokenize(const Options& options, shared_ptr<const Source> source)->Result
{
	auto code = string_view(source->getCode());
	auto offset = size_t(0);
	
	auto ts = make_shared<TokenStream>(source);
	auto result = Result{ts};
	auto info = ParseInfo{ts};
	auto parserState = State{info};

	auto state = ParseState{[]() {
		Finish state;
		addExpectationsForSegment(state, Segment::None);
		return state;
	}()};
	auto reporter = ([&]()->Reporter {
		if (!options.reporter.hasImpl() && options.errorReporting) {
			Reporter reporter;
			reporter.setImpl([source](const ReportData& log)->void {
				using namespace fmt::literals;

				auto stream = log.type == ReportType::Fatal || log.type == ReportType::Error ? stdout : stdout;
				auto reportType = string_view{[](ReportType type) {
					switch (type) {
					case ReportType::Fatal:
						return "fatal"sv;
					case ReportType::Error:
						return "error"sv;
					case ReportType::Warning:
						return "warn"sv;
					case ReportType::Info:
						return "info"sv;
					}
					return "unknown"sv;
				}(log.type)};

				auto& report = std::any_cast<const Report&>(log.data);
				auto& lineInfo = source->getLineInfo(source->getLineIndexByOffset(report.token.offset));
				auto lineNum = to_string(lineInfo.number);
				auto lineEnd = lineInfo.offset + lineInfo.length;
				auto tokenEnd = report.token.offset + report.token.text.size();

				fmt::print(stream, fmt::emphasis::bold | fg(fmt::color::red), "{}[E{:04}]", reportType, report.diagnosis.getCodeInt());
				fmt::print(stream, fmt::text_style{fmt::emphasis::bold}, ": {}\n", report.diagnosis.getName());
				fmt::print(stream, fg(fmt::color::blue), "{:>{}}", "--> ", lineNum.size() + 4);
				fmt::print(
					stream,
					"{file}:{line}:{column}\n",
					"file"_a = source->getName(),
					"line"_a = lineNum,
					"column"_a = source->getColumnByOffset(report.token.offset)
				);
				fmt::print(stream, fg(fmt::color::blue), "{} |  ", lineNum);

				if (report.token.offset > 0) {
					fmt::print(stream, "{}", source->getText(lineInfo.offset, report.token.offset - lineInfo.offset));
				}

				if (tokenEnd < lineEnd) {
					fmt::print(stream, fg(fmt::color::red), "{}", report.token.text);
					fmt::print(stream, "{}\n", source->getText(tokenEnd, lineEnd - tokenEnd));
				}
				else {
					fmt::print(stream, fg(fmt::color::red), "{}\n", report.token.text);
				}

				fmt::print(
					stream,
					fg(fmt::color::red),
					"{:>{}}{:^>{}} {}\n\n",
					"",
					report.token.offset + lineNum.size() + 4,
					"^",
					report.token.text.size(),
					report.diagnosis.getMessage()
				);
			});
			return reporter;
		}
		return options.reporter;
	})();

	auto step = [=, &result, &parserState](ParseState state, size_t offset, optional<LexResult> res)->ParseState {
		if (!res) {
			auto delimitedToken = source->getToken(offset);
			return Fatal{move(delimitedToken), diagnose<DiagCode::UnexpectedLexeme>()};
		}

		auto token = Token(source.get(), res->type, offset, res->length);

		if (res->type != TokenType::WhiteSpace) {
			if (ts->empty() || res->type != TokenType::EOL || !ts->back().is(TokenType::EOL)) {
				return std::visit(visitor{
					[&](Continue&& newState)->ParseState {
						if (auto cont = get_if<Continue>(&state)) {
							cont->add(newState.token);
							return state;
						}
						return newState;
					},
					[&](Finish&& newState)->ParseState {
						auto nextState = ([&]()->ParseState {
							auto parseLineState = ParseState();
							if (auto cont = get_if<Continue>(&state)) {
								if (newState.token) cont->add(*newState.token);
								
								parseLineState = parseLine(parserState, move(*cont));
								
								if (auto parseLineFinished = get_if<Finish>(&parseLineState)) {
									return newState.merge(*parseLineFinished);
								}
								return parseLineState;
							}
							return parseLine(parserState, move(newState));
						})();

						if (auto finish = get_if<Finish>(&nextState)) {
							if (!finish->expected)
								addExpectationsForSegment(*finish, parserState.segment);
						}

						if (auto finish = get_if<Finish>(&state)) {
							if (finish->expected && !options.testForceTokenization) {
								if (auto error = getExpectedTokenError(*finish->expected, token))
									return finish->report(*error);
							}
						}

						return nextState;
					},
					[](Fatal&& newState)->ParseState {
						return newState;
					}
				}, parseToken(state, move(token)));
			}
		}

		return state;
	};

	auto reportState = [&](ParseState&& state)->ParseState {
		if (auto fatal = get_if<Fatal>(&state)) {
			++result.numErrors;
			result.reports.emplace_back(fatal->error);
			result.hadFatal = true;
			reporter.fatal(fatal->error);
			return state;
		}

		if (auto finish = get_if<Finish>(&state)) {
			for (auto& report : finish->reports) {
				reporter.report(report.type, report);

				if (report.type == ReportType::Error || report.type == ReportType::Fatal) {
					++result.numErrors;

					if (report.type == ReportType::Fatal)
						result.hadFatal = true;
				}
			}

			result.reports.insert(result.reports.end(), make_move_iterator(finish->reports.begin()), make_move_iterator(finish->reports.end()));
		}
		return state;
	};

	while (offset < code.size()) {
		auto res = lexOneOf(code.substr(offset), lexRules);
		state = reportState(step(state, offset, res));

		if (result.hadFatal) break;

		offset += res->length;
	}

	if (ts->empty() || !ts->back().is(TokenType::EOL)) {
		if (!result.hadFatal) {
			auto res = LexResult{TokenType::EOL, 0};
			state = reportState(step(state, offset, res));
		}

		ts->push(source.get(), TokenType::EOL, offset, code.substr(offset, 0));
	}

	return result;
}

}