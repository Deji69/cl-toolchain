#pragma once
#include <catch.hpp>
#include <initializer_list>
#include <CLARA/Parser.h>
#include <CLARA/Source.h>

using namespace CLARA;
using namespace CLARA::CLASM;

inline auto getParseOpts(bool forceTokenization = false) {
	// disabling reporting errors to stdout/stderr as it will mess with test results
	auto options = Parser::Options{};
	options.errorReporting = false;
	options.testForceTokenization = forceTokenization;
	return options;
}

template<typename T>
inline auto isOneOf(const T& value, std::initializer_list<T> oneOfThese) {
	for (auto& one : oneOfThese) {
		if (value == one)
			return true;
	}
	return false;
}

inline auto checkResult(const Parser::Result& res) {
	for (auto& report : res.reports) {
		UNSCOPED_INFO(string{report.diagnosis.getName()} +" ["s + to_string(report.diagnosis.getCodeInt()) + "]: "s + string{report.token.text});
	}
	return res.ok();
}

struct ParsingTestHelper {
	Parser::Options options;
	Parser::Result result;
	TokenStream* tokensPtr;

	ParsingTestHelper(bool forceTokenization = false) : options(getParseOpts(forceTokenization)) {}

	auto parse(string code) {
		return Parser::tokenize(options, make_shared<Source>("test", code));
	}

	auto parseCode(string code) {
		return parse(".code\n" + code);
	}

	auto parseData(string code) {
		return parse(".data\n" + code);
	}

	auto& parseExpect(string code, const map<Segment::Type, initializer_list<TokenType>>& segTypes) {
		result = parse(std::move(code));
		auto i = 0_uz;
		for (auto& [segment, types] : segTypes) {
			auto& tokens = *result.info.segments[segment].tokens;
			REQUIRE(tokens.size() >= types.size());
			for (auto type : types) {
				CHECK(tokens[i++].type == type);
			}
		}
		return *tokensPtr;
	}

	auto& parseExpectCode(string code, initializer_list<TokenType> types) {
		result = parseCode(std::move(code));
		auto& tokens = *result.info.segments[Segment::Code].tokens;
		REQUIRE(tokens.size() >= types.size());
		auto i = 0_uz;
		for (auto type : types) {
			CHECK(tokens[i++].type == type);
		}
		return tokens;
	}
};