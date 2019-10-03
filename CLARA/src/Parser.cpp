#include <CLARA/pch.h>
#include <CLARA/Parser.h>

using namespace CLARA::CLASM;

namespace CLARA::CLASM::Parser {

struct LexRule {
	TokenType type;
	size_t(*func)(string_view);
};

struct LexResult {
	TokenType type;
	size_t length;

	LexResult(TokenType type, size_t length) : type(type), length(length)
	{ }
};

struct Continue {
	using Tokens = small_vector<Token, 16>;
	Tokens tokens;

	Continue() = default;

	Continue(Token token) : tokens({token})
	{ }

	Continue(Token&& token) : tokens({token})
	{ }

	auto add(const Token& token)
	{
		tokens.emplace_back(token);
	}
	
	auto add(const Tokens& newTokens)
	{
		tokens.insert(tokens.end(), newTokens.begin(), newTokens.end());
	}
};

struct Finish {
	optional<Token> token;
	optional<Expected> expected;
	small_vector<Report> reports;

	Finish() = default;

	Finish(Token&& token) : token(token)
	{ }

	Finish(const Token& token) : token(token)
	{ }

	auto& merge(const Finish& other)
	{
		if (reports.empty())
			reports = move(other.reports);
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
	small_vector<Token, 16> tokens;

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
using ParseResult = variant<Success, Error, small_vector<Error>>;

struct State {
	ParseInfo& info;
	ParseState state;
	TokenStream* tokens;                        // points to the active segment tokens: &info.segments[segment].tokens
	small_vector<Token*> unresolvedLabelTokens;
	std::unordered_multimap<string, size_t> unresolvedLabelTokenNameMap;
	Segment::Type segment = Segment::Header;
	uint64_t offset = 0;

	State(shared_ptr<const Source> source, ParseInfo& info_, ParseState state_) : info(info_), state(state_)
	{
		auto segType = 0;
		for (auto& segment : info.segments) {
			segment.type = static_cast<Segment::Type>(segType++);
			segment.tokens = make_shared<TokenStream>(source);
		}

		tokens = info.segments[Segment::Header].tokens.get();
	}

	auto setSegment(Segment::Type seg)
	{
		segment = seg;
		tokens = info.segments[seg].tokens.get();
	}

	auto defineLabel(string name, Token& token, Segment::Type segment)->pair<Label&, bool>
	{
		auto idx = info.labels.size();
		auto res = info.labelMap.emplace(name, idx);
		auto label = res.second
			? info.labels.emplace_back(make_unique<Label>(Label{name, token, segment})).get()
			: info.labels[res.first->second].get();
		auto [begin, end] = unresolvedLabelTokenNameMap.equal_range(name);
		
		for (auto it = begin; it != end; ++it) {
			unresolvedLabelTokens[it->second]->annotation.emplace<LabelRef>(info.labels[it->second].get());
		}

		if (res.second)
			label->segment = segment;

		token.annotation.emplace<const Label*>(label);
		unresolvedLabelTokenNameMap.erase(begin, end);
		return pair<Label&, bool>(*label, res.second);
	}

	auto referenceLabel(Token& token)
	{
		auto it = info.labelMap.find(string(token.text));
		if (it != info.labelMap.end()) {
			token.annotation.emplace<LabelRef>(info.labels[it->second].get());
		}
		else {
			unresolvedLabelTokenNameMap.emplace(get<string>(token.annotation), unresolvedLabelTokens.size());
			unresolvedLabelTokens.push_back(&token);
		}
	}
};

auto parseIdentifier(const State& state, Token&& token)->ParseState
{
	auto id = string(token.text);

	auto getKeyword = [&]() {
		if (auto kw = Keyword::fromName(id); kw != Keyword::MAX) {
			token.type = TokenType::Keyword;
			token.annotation.emplace<Keyword::Type>(kw);
			return true;
		}
		return false;
	};

	auto getDataType = [&]() {
		if (auto dt = DataType::fromName(id); dt != DataType::MAX) {
			token.type = TokenType::DataType;
			token.annotation.emplace<DataType::Type>(dt);
			return true;
		}
		return false;
	};

	auto getMnemonic = [&]() {
		if (auto mnemonic = Mnemonic::fromName(id); mnemonic != Mnemonic::MAX) {
			token.type = TokenType::Mnemonic;
			token.annotation.emplace<Mnemonic::Type>(mnemonic);
			return true;
		}
		return false;
	};

	auto getInstruction = [&]() {
		if (auto insn = Instruction::fromName(id); insn != Instruction::MAX) {
			token.type = TokenType::Instruction;
			token.annotation.emplace<Instruction::Type>(insn);
			return true;
		}
		return false;
	};

	if (state.segment == Segment::Data) {
		if (getDataType())
			return Continue(token);
	}
	
	if (
		getKeyword() ||
		getMnemonic() ||
		getInstruction()
	)
		return Continue(token);

	token.annotation.emplace<string>(move(id));

	if (is<Continue>(state.state))
		return Continue(token);
	return Finish(token);
}

auto parseSegment(const State& state, Token&& token)->ParseState
{
	auto result = Finish(move(token));

	if (is<Continue>(state.state) && state.segment != Segment::Data)
		return result.error(token, diagnose<DiagCode::UnexpectedSegmentAfterTokens>());

	auto id = token.text.substr(1);

	if (auto segment = Segment::fromName(id); segment != Segment::MAX) {
		result.token->annotation = segment;
		return result;
	}

	return Finish().error(token, diagnose<DiagCode::InvalidSegment>());
}

auto parseString(const State&, Token&& token)->ParseState
{
	auto result = Finish{};
	auto str = string{};
	str.reserve(token.text.size());

	// get just the stuff between the quotes
	auto text = token.text.substr(1_uz, token.text.size() - 2);
	auto l = 0_uz;

	for (auto i = text.find_first_of('\\'); i != text.npos; l = i, i = text.find_first_of('\\', i)) {
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

auto resolveIntegerAnnotation(Token& token, uint64_t num, bool negative)->TokenAnnotation
{
	if (!negative) {
		if (num <= static_cast<uint8>(std::numeric_limits<int8>::max()))
			return static_cast<int8>(num);
		else if (num <= std::numeric_limits<uint8>::max())
			return static_cast<uint8>(num);
		else if (num <= static_cast<uint16>(std::numeric_limits<int16>::max()))
			return static_cast<int16>(num);
		else if (num <= std::numeric_limits<uint16>::max())
			return static_cast<uint16>(num);
		else if (num <= static_cast<uint32>(std::numeric_limits<int32>::max()))
			return static_cast<int32>(num);
		else if (num <= std::numeric_limits<uint32>::max())
			return static_cast<uint32>(num);
		else if (num <= static_cast<uint64>(std::numeric_limits<int64>::max()))
			return static_cast<int64>(num);
		else if (num <= std::numeric_limits<uint64>::max())
			return static_cast<uint64>(num);
	}
	else {
		if (num <= static_cast<uint8>(std::numeric_limits<int8>::max()))
			return static_cast<int8>(-static_cast<int8>(num));
		else if (num <= std::numeric_limits<uint8>::max())
			return static_cast<int16>(-static_cast<int16>(num));
		else if (num <= static_cast<uint16>(std::numeric_limits<int16>::max()))
			return static_cast<int16>(-static_cast<int16>(num));
		else if (num <= std::numeric_limits<uint16>::max())
			return static_cast<int32>(-static_cast<int32>(num));
		else if (num <= static_cast<uint32>(std::numeric_limits<int32>::max()))
			return static_cast<int32>(-static_cast<int32>(num));
		else if (num <= std::numeric_limits<uint32>::max())
			return static_cast<int64>(-static_cast<int64>(num));
		else if (num <= static_cast<uint64>(std::numeric_limits<int64>::max()))
			return static_cast<int64>(-static_cast<int64>(num));
	}
	return TokenAnnotation{};
}

auto parseNumeric(const State& state, Token&& token)->ParseState
{
	auto literal = token.text;
	auto negative = literal[0] == '-';
	
	if (negative || literal[0] == '+')
		literal = literal.substr(1);
	
	if (token.type == TokenType::HexLiteral) {
		if (auto num = stringToInt<uint64_t>(literal.substr(2), 16)) {
			token.annotation = resolveIntegerAnnotation(token, *num, negative);
		}
	}
	else if (token.type == TokenType::IntegerLiteral) {
		if (auto num = stringToInt<uint64_t>(literal, 10)) {
			token.annotation = resolveIntegerAnnotation(token, *num, negative);
		}
	}
	else if (token.type == TokenType::FloatLiteral) {
		if (auto num = stringToFloat(token.text)) {
			token.annotation = *num;
		}
	}
	
	Finish result;

	token.type = TokenType::Numeric;

	if (is<monostate>(token.annotation)) {
		result.error(token, diagnose<DiagCode::InvalidNumericLiteral>());
	}
	else if (is<Continue>(state.state)) {
		return Continue(token);
	}

	result.token = move(token);
	return result;
}

auto preParseToken(const State& state, const Token& token)->ParseState
{
	if (state.segment == Segment::Data && is<Continue>(state.state)) {
		switch (token.type) {
		default: break;
		case TokenType::Segment: return Finish();
		}
	}
	return Continue();
}

auto parseToken(const State& state, Token&& token)->ParseState
{
	switch (token.type) {
	default: break;
	case TokenType::EndOfLine:
		if (state.segment == Segment::Data && is<Continue>(state.state)) {
			return Continue();
		}
	case TokenType::EndOfFile:
		return Finish();
	case TokenType::Identifier: return parseIdentifier(state, forward<Token>(token));
	case TokenType::Segment: return parseSegment(state, forward<Token>(token));
	case TokenType::String: return parseString(state, forward<Token>(token));
	case TokenType::HexLiteral:
	case TokenType::FloatLiteral:
	case TokenType::IntegerLiteral:
		return parseNumeric(state, forward<Token>(token));
	case TokenType::Label:
		if (state.segment == Segment::Data) {
			return Continue(token);
		}
		if (is<Continue>(state.state)) {
			return Finish().error(forward<Token>(token), diagnose<DiagCode::UnexpectedLabelAfterTokens>());
		}
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
		if (!token.is(TokenType::Numeric))
			return Error{token, diagnose<DiagCode::InvalidOperandType>(type)};
		break;

	case OperandType::REL32:
		if (token.type != TokenType::LabelRef)
			return Error{token, diagnose<DiagCode::InvalidOperandType>(type)};
		break;

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

auto parseInstructionLine(State& state, const TokenVec& tokens)->ParseResult
{
	auto parseOperands = [&](Instruction::Type instruction)->ParseResult {
		auto success = Success();
		auto& operands = Instruction::getOperands(instruction);
		auto begin = std::begin(tokens) + 1;
		auto end = std::end(tokens);
		auto it = begin;

		success.tokens.reserve(static_cast<size_t>(std::abs(std::distance(it, end) + 1)));

		auto& insnToken = success.addToken(tokens[0].source, TokenType::Instruction, tokens[0].offset, tokens[0].text.size());
		insnToken.annotation = instruction;

		auto parseOperand = [&success](OperandType type, Token token)->ParseResult {
			if (token.type == TokenType::Identifier) {
				switch (type) {
				default: break;
				case OperandType::REL32:
					token.type = TokenType::LabelRef;
					break;
				}
			}

			auto res = checkOperandType(type, token);

			if (is<Error>(res)) {
				return get<Error>(res);
			}

			success.tokens.emplace_back(move(token));
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
						auto token = begin != end ? Source::Token(*begin, *std::prev(it)) : Source::Token(tokens.front());
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

		return success;
	};

	if (tokens[0].type == TokenType::Mnemonic) {
		auto mnemonic = get<Mnemonic::Type>(tokens[0].annotation);

		for (auto& overload : Mnemonic::getOverloads(mnemonic)) {
			auto res = parseOperands(overload.insn);

			if (auto success = get_if<Success>(&res)) {
				return *success;
			}
		}

		return Error{Source::Token(tokens.front(), tokens.back()), diagnose<DiagCode::InvalidMnemonicOperands>(mnemonic)};
	}
	return parseOperands(get<Instruction::Type>(tokens[0].annotation));
}

auto parseGlobalKeywordLine(State&, const TokenVec& tokens)->ParseResult
{
	constexpr auto numParams = 1_uz;
	auto success = Success();
	auto numArgs = tokens.size() - 1;

	if (numArgs >= numParams) {
		success.addToken(move(tokens[0]));
		
		auto errors = small_vector<Error, 8, 16>();
		
		for (auto it = tokens.cbegin() + 1; it != tokens.cend(); ++it) {
			if (it->type != TokenType::Identifier) {
				errors.emplace_back(Error{*it, diagnose<DiagCode::ExpectedToken>(it->type, TokenType::Label)});
			}

			auto& token = success.addToken(move(*it));
			token.type = TokenType::LabelRef;
		}

		if (!errors.empty()) {
			return errors;
		}
		
		return success;
	}

	auto token = numArgs > 1 ? Source::Token(tokens[1], tokens.back()) : Source::Token(tokens.front());
	return Error{token, diagnose<DiagCode::InvalidKeywordArgCount>(Keyword::Global, numParams, numArgs)};
}

auto parseKeywordLine(State& state, const TokenVec& tokens)->ParseResult
{
	switch (get<Keyword::Type>(tokens[0].annotation)) {
	case Keyword::Global: return parseGlobalKeywordLine(state, tokens);
	case Keyword::Extern:
	case Keyword::Import:
	case Keyword::Include:
	case Keyword::MAX:
		break;
	}
	return Error{tokens[0], diagnose<DiagCode::InvalidIdentifier>()};
}

auto parseDataDeclarationLine(State& parser, Continue&& state)->ParseState
{
	if (state.tokens.empty())
		return Finish();

	auto& tokens = state.tokens;
	auto res = ParseResult(Success());

	if (tokens[0].type == TokenType::Label) {
		auto name = string(tokens[0].text.substr(0, tokens[0].text.size() - 1));
		auto token = parser.tokens->push(move(tokens[0]));
		auto res = parser.defineLabel(name, *token, Segment::Data);

		for (auto it = tokens.begin() + 1; it != tokens.end(); ++it) {
			parser.tokens->push(move(*it));
		}

		if (!res.second) {
			return Finish().error(*token, diagnose<DiagCode::LabelRedefinition>(res.first));
		}
	}
	return Finish().expect(TokenType::EndOfLine);
}

auto parseLine(State& parser, Finish&& state)->ParseState
{
	if (!state.token) {
		return move(state);
	}

	auto token = &*state.token;

	switch (state.token->type) {
	default: break;

	case TokenType::Identifier:
		return Finish().error(*parser.tokens->push(move(*token)), diagnose<DiagCode::InvalidIdentifier>());

	case TokenType::Segment:
		parser.setSegment(get<Segment::Type>(token->annotation));
		return Finish().expect(TokenType::EndOfLine);

	case TokenType::Label:
		{
			auto name = string(state.token->text.substr(0, state.token->text.size() - 1));
			token = &*parser.tokens->push(move(*token));

			auto&& [label, defined] = parser.defineLabel(name, *token, parser.segment);

			if (!defined) {
				return Finish().error(*token, diagnose<DiagCode::LabelRedefinition>(label));
			}
		}
		return Finish();

	case TokenType::Instruction:
		parser.tokens->push(move(*token));
		return Finish().expect({TokenType::EndOfLine, make_pair(TokenType::Separator, ","s)});
	}

	parser.tokens->push(move(*token));
	return Finish();
}

auto parseLine(State& parser, Continue&& state)->ParseState
{
	if (parser.segment == Segment::Data)
		return parseDataDeclarationLine(parser, std::move(state));

	if (state.tokens.empty())
		return Finish();

	auto& tokens = state.tokens;

	auto seperable = false;
	auto res = ParseResult(Success());

	switch (tokens[0].type) {
	case TokenType::Mnemonic:
	case TokenType::Instruction:
		res = parseInstructionLine(parser, tokens);
		seperable = true;
		break;

	case TokenType::Keyword:
		res = parseKeywordLine(parser, tokens);
		break;

	case TokenType::Label:
	case TokenType::Segment:
		throw ParseException("unexpected token in continue state");

	default:
		return Fatal(tokens[0], diagnose<DiagCode::UnexpectedTokenBeganLine>());
	}

	if (is<Error>(res)) {
		return Finish().error(tokens[0], move(get<Error>(res).info));
	}

	for (auto& token : get<Success>(res).tokens) {
		auto addedToken = parser.tokens->push(move(token));

		if (addedToken->type == TokenType::LabelRef) {
			parser.referenceLabel(*addedToken);
		}
	}

	if (seperable)
		return Finish().expect({TokenType::EndOfLine, make_pair(TokenType::Separator, ","s)});
	return Finish().expect(TokenType::EndOfLine);
}

auto parseFinish(State& state)->ParseState
{
	auto fin = Finish();

	if (!state.unresolvedLabelTokens.empty()) {
		for (auto i = 0_uz; i < state.unresolvedLabelTokenNameMap.bucket_count(); ++i) {
			auto it = state.unresolvedLabelTokenNameMap.begin(i);
			if (it == state.unresolvedLabelTokenNameMap.end(i))
				continue;
			auto& elem = *state.unresolvedLabelTokenNameMap.begin(i);
			auto token = state.unresolvedLabelTokens[elem.second];
			fin.error(*token, diagnose<DiagCode::UnresolvedLabelReference>());
		}
	}

	return fin;
}

auto lexComment(string_view sv)
{
	if (sv[0] == ';') {
		auto pos = sv.find('\n', 1);
		if (pos != sv.npos) return pos;
	}
	return 0_uz;
}

auto lexWhitespace(string_view sv)
{
	auto it = std::find_if_not(sv.cbegin(), sv.cend(), [](auto c) {
		return std::isspace(static_cast<unsigned char>(c));
	});
	return static_cast<size_t>(std::distance(sv.cbegin(), it));
}

auto lexWhitespaceNoNewlines(string_view sv)
{
	auto it = std::find_if_not(sv.cbegin(), sv.cend(), [](auto c) {
		return c != '\n' && std::isspace(static_cast<unsigned char>(c));
	});
	return static_cast<size_t>(std::distance(sv.cbegin(), it));
}

auto lexNewline(string_view sv)
{
	auto it = std::find_if_not(sv.cbegin(), sv.cend(), [](auto c) {
		return c == '\n';
	});
	return static_cast<size_t>(std::distance(sv.cbegin(), it));
}

auto lexIdentifier(string_view sv)
{
	if ((std::isalpha(sv[0]) || sv[0] == '_') && sv.size() > 1) {
		auto it = std::find_if_not(sv.cbegin() + 1, sv.cend(), [](auto c) {
			return c == '_' || std::isalnum(c);
		});
		if (it == sv.cend())
			return sv.size();
		return static_cast<size_t>(std::distance(sv.cbegin(), it));
	}
	return 0_uz;
}

auto lexSegment(string_view sv)
{
	if (sv[0] == '.') {
		if (auto res = lexIdentifier(sv.substr(1))) {
			return static_cast<size_t>(res) + 1;
		}
	}
	return 0_uz;
}

auto lexLabel(string_view sv)
{
	if (auto res = lexIdentifier(sv)) {
		if (res < sv.size() && sv[res] == ':') {
			++res;
			if (res == sv.size() || std::isspace(sv[res]))
				return res;
		}
	}
	return 0_uz;
}

auto lexHexLiteral(string_view sv)
{
	static std::regex hexRegex("^([+\\-]?0x[\\dA-Fa-f]+)\\b");
	auto s = string(sv);
	std::smatch sm;

	if (std::regex_search(s, sm, hexRegex)) {
		return static_cast<size_t>(sm[0].length());
	}
	return 0_uz;
}

auto lexIntegerLiteral(string_view sv)
{
	static std::regex integerRegex{"^([+\\-]?(?:0|[1-9]\\d*))(?:\\b[^\\.]|$)"};
	auto s = string(sv);
	std::smatch sm;
	
	if (std::regex_search(s, sm, integerRegex)) {
		return static_cast<size_t>(sm[1].length());
	}
	return 0_uz;
}

auto lexFloatLiteral(string_view sv)
{
	static std::regex floatRegex{"^[+\\-]?(?:0|[1-9]\\d*)(?:\\.\\d*)(?:[eE][+\\-]?\\d+)?\\b"};
	auto s = string(sv);
	std::smatch sm;
	
	if (std::regex_search(s, sm, floatRegex)) {
		return static_cast<size_t>(sm[0].length());
	}
	return 0_uz;
}

auto lexString(string_view sv)
{
	if (sv[0] == '"') {
		size_t end = 0;

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
	return 0_uz;
}

auto lexSeparator(string_view sv)
{
	switch (sv[0]) {
	case '=':
	case ':':
	case ',':
		return 1_uz;
	}
	return 0_uz;
}

auto lexOne(string_view sv, LexRule rule)->optional<LexResult>
{
	if (auto len = rule.func(sv))
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
	{TokenType::EndOfLine, lexNewline},
	{TokenType::WhiteSpace, lexComment},
	{TokenType::WhiteSpace, lexWhitespaceNoNewlines},
	{TokenType::Separator, lexSeparator},
	{TokenType::Segment, lexSegment},
	{TokenType::String, lexString},
	{TokenType::HexLiteral, lexHexLiteral},
	{TokenType::IntegerLiteral, lexIntegerLiteral},
	{TokenType::FloatLiteral, lexFloatLiteral},
	{TokenType::Label, lexLabel},
	{TokenType::Identifier, lexIdentifier},
};

auto getExpectedTokenError(const Expected& expect, const Token& token)->optional<Report>
{
	return std::visit(visitor{
		[&](TokenType type)->optional<Report> {
			if (token.type != type) {
				return Report::error(token, diagnose<DiagCode::ExpectedToken>(token.type, type));
			}
			return nullopt;
		},
		[&](const TokenAndString& typeAndValue)->optional<Report> {
			if (token.type != typeAndValue.first || token.text != typeAndValue.second) {
				return Report::error(token, diagnose<DiagCode::ExpectedToken>(token.type, typeAndValue));
			}
			return nullopt;
		},
		[&](const AnyOf& types)->optional<Report> {
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

auto addExpectationsForSegment(Segment::Type segment, Finish& state)->Finish&
{
	switch (segment) {
	case Segment::MAX:
	case Segment::Header: return state.expect({TokenType::EndOfFile, TokenType::EndOfLine, TokenType::Identifier, TokenType::Segment});
	case Segment::Code: return state.expect({TokenType::EndOfFile, TokenType::EndOfLine, TokenType::Identifier, TokenType::Label, TokenType::Segment});
	case Segment::Data: return state.expect({TokenType::EndOfFile, TokenType::EndOfLine, TokenType::Label, TokenType::Segment});
	}
	return state;
}

auto addExpectationsForSegment(Segment::Type segment)->Finish
{
	auto state = Finish{};
	addExpectationsForSegment(segment, state);
	return state;
}

auto tokenize(const Options& options, shared_ptr<const Source> source)->Result
{
	const auto code = string_view(source->getCode());
	auto offset = 0_uz;
	
	auto result = Result{};
	auto parserState = State{
		source,
		result.info,
		addExpectationsForSegment(Segment::MAX)
	};
	const auto& tokens = parserState.tokens;       // intentional reference-to-pointer: parserState.tokens will update
	
	result.info.labels.reserve(500);
	parserState.unresolvedLabelTokens.reserve(100);

	const auto reporter = ([&]()->Reporter {
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
				auto& lineInfo = source->getLineInfo(
					source->getLineIndexByOffset(static_cast<uint>(report.token.offset))
				);
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
					"column"_a = source->getColumnByOffset(static_cast<uint>(report.token.offset))
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
	
	auto reportState = [&](ParseState&& state)->ParseState&& {
		if (auto fatal = get_if<Fatal>(&state)) {
			++result.numErrors;
			result.reports.emplace_back(move(fatal->error));
			result.hadFatal = true;
			reporter.fatal(fatal->error);
			return move(state);
		}

		if (auto finish = get_if<Finish>(&state)) {
			if (!finish->reports.empty()) {
				for (auto& report : finish->reports) {
					reporter.report(report.type, report);

					if (report.type == ReportType::Error || report.type == ReportType::Fatal) {
						++result.numErrors;

						if (report.type == ReportType::Fatal)
							result.hadFatal = true;
					}
				}

				result.reports.insert(result.reports.end(), make_move_iterator(finish->reports.begin()), make_move_iterator(finish->reports.end()));
				finish->reports.clear();
			}
		}
		return move(state);
	};

	auto step = [&](size_t offset, optional<LexResult> res)->ParseState {
		if (!res) {
			auto delimitedToken = source->getToken(offset);
			return reportState(
				Fatal{move(delimitedToken), diagnose<DiagCode::UnexpectedLexeme>()}
			);
		}

		auto token = Token(source.get(), res->type, offset, res->length);

		if (res->type != TokenType::WhiteSpace) {
			if (tokens->empty() || res->type != TokenType::EndOfLine || !tokens->back().is(TokenType::EndOfLine)) {
				auto postParseVisitor = visitor{
					[&](Continue&& newState)->ParseState {
						if (auto cont = get_if<Continue>(&parserState.state)) {
							cont->add(newState.tokens);
							return parserState.state;
						}
						return move(newState);
					},
					[&](Finish&& newState)->ParseState {
						auto nextState = ([&]()->ParseState {
							if (auto cont = get_if<Continue>(&parserState.state)) {
								if (newState.token) cont->add(*newState.token);

								auto parseLineState = parseLine(parserState, move(*cont));

								if (auto parseLineFinished = get_if<Finish>(&parseLineState)) {
									return newState.merge(*parseLineFinished);
								}
								return parseLineState;
							}
							return parseLine(parserState, move(newState));
						})();

						if (auto finish = get_if<Finish>(&nextState)) {
							if (!finish->expected)
								addExpectationsForSegment(parserState.segment, *finish);
						}

						if (auto finish = get_if<Finish>(&parserState.state)) {
							if (finish->expected && !options.testForceTokenization) {
								if (auto error = getExpectedTokenError(*finish->expected, token))
									return finish->report(*error);
							}
						}

						return nextState;
					},
					[](Fatal&& newState)->ParseState {
						return move(newState);
					}
				};
				if (is<Continue>(parserState.state)) {
					parserState.state = reportState(
						std::visit(postParseVisitor, preParseToken(parserState, token))
					);
				}
				return reportState(
					std::visit(postParseVisitor, parseToken(parserState, move(token)))
				);
			}
		}

		return reportState(move(parserState.state));
	};

	while (offset < code.size()) {
		const auto res = lexOneOf(code.substr(offset), lexRules);
		parserState.state = step(offset, res);

		if (result.hadFatal) break;

		offset += res->length;
	}

	if (!result.hadFatal) {
		auto const activeSegment = parserState.segment;

		for (auto& segment : result.info.segments) {
			if (!segment.tokens->empty() && segment.tokens->back().type != TokenType::EndOfLine) {
				parserState.setSegment(segment.type);
				parserState.state = reportState(step(offset, LexResult{TokenType::EndOfLine, 0}));
			}
		}

		parserState.setSegment(activeSegment);
		parserState.state = reportState(step(offset, LexResult{TokenType::EndOfFile, 0}));

	}

	tokens->push(source.get(), TokenType::EndOfFile, offset, code.substr(offset, 0));

	parserState.state = reportState(parseFinish(parserState));

	return result;
}

}