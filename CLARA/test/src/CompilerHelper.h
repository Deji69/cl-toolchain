#pragma once
#include <catch.hpp>
#include <initializer_list>
#include <CLARA/Compiler.h>
#include <CLARA/IBinaryOutput.h>
#include <CLARA/Parser.h>
#include <CLARA/Source.h>

using namespace CLARA;
using namespace CLARA::CLASM;

struct MockOutputHandler : public IBinaryOutput {
	vector<uint8_t> output;

	virtual ~MockOutputHandler() = default;

	template<typename T>
	auto check(const T& cont) const->bool
	{
		if (cont.size() != output.size()) {
			UNSCOPED_INFO(fmt::format("Size is {}, expected {}", output.size(), cont.size()));
			return false;
		}
		
		auto it = output.cbegin();
		
		for (auto expectIt = cont.begin(); expectIt != cont.end(); ++expectIt, ++it) {
			if (*expectIt != *it) {
				UNSCOPED_INFO(fmt::format(
					"Mismatch output[{}] == {:#x}, expected {:#x}",
					std::distance(output.cbegin(), it),
					*it,
					*expectIt
				));
				return false;
			}
		}
		return true;
	}

	virtual auto write(const uint8_t* begin, const uint8_t* end)->void override
	{
		output.insert(output.end(), begin, end);
	}
};

inline auto makeParseInfo(const vector<pair<TokenType, TokenAnnotation>>& tokenStreamInit)
{
	Parser::ParseInfo info;
	const auto tokens = TokenStream{tokenStreamInit};
	auto* segmentPtr = &info.segments[Segment::Header];
	for (auto it = tokens.begin(); it != tokens.end(); ++it) {
		if (it->type == TokenType::Segment) {
			segmentPtr = &info.segments[get<Segment::Type>(it->annotation)];
			if (!segmentPtr->tokens)
				segmentPtr->tokens = std::make_shared<TokenStream>();
		}
		else
			segmentPtr->tokens->push(move(*it));
	}
	return info;
}

inline auto makeParseInfo(initializer_list<TokenAnnotation> tokenStreamInit)
{
	auto vec = small_vector<pair<TokenType, TokenAnnotation>>();

	for (auto& annotation : tokenStreamInit) {
		auto type = getAnnotationTokenType(annotation);
		vec.push_back(make_pair(type, annotation));
	}
	return makeParseInfo(vec);
}