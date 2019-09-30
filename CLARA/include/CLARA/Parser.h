#pragma once
#include <CLARA/Assembly.h>
#include <CLARA/Common.h>
#include <CLARA/Diagnostic.h>
#include <CLARA/Reporter.h>
#include <CLARA/Source.h>
#include <CLARA/TokenStream.h>
#include <CLARA/Label.h>

namespace CLARA::CLASM::Parser {

struct ParseException : std::runtime_error {
	ParseException(const char* msg) : std::runtime_error(msg)
	{ }
};

struct Report {
	ReportType type;
	Source::Token token;
	Diagnosis diagnosis;

	Report() = default;
	Report(ReportType type, Source::Token token, Diagnosis&& diagnosis);

	static auto info(const Source::Token&, Diagnosis&&)->Report;
	static auto warning(const Source::Token&, Diagnosis&&)->Report;
	static auto error(const Source::Token&, Diagnosis&&)->Report;
	static auto fatal(const Source::Token&, Diagnosis&&)->Report;
};

struct SegmentInfo {
	TokenStream::const_iterator begin;
	TokenStream::const_iterator end;
};

struct ParseInfo {
	shared_ptr<TokenStream> tokens;
	vector<unique_ptr<Label>> labels;
	unordered_map<string, size_t> labelMap;
	array<small_vector<SegmentInfo>, Segment::MAX> segments;
};

struct Result {
	ParseInfo info;
	small_vector<Report, 16, 128> reports;
	size_t numWarnings = 0;
	size_t numErrors = 0;
	bool hadFatal = false;

	inline operator bool() const
	{
		return ok();
	}

	inline auto ok() const->bool
	{
		return !numErrors;
	}
};

struct Options {
	Reporter reporter;
	bool errorReporting = true;
	bool testForceTokenization = false;                  // Disables errors that may prevent tokenization
};

auto tokenize(const Options& options, shared_ptr<const Source> source)->Result;

}