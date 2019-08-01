#pragma once
#include <CLASM/Common.h>
#include <CLASM/TokenStream.h>
#include <CLASM/Assembly.h>
#include <CLASM/Reporter.h>
#include <CLASM/Diagnostic.h>
#include <CLASM/Source.h>

namespace CLARA::CLASM::Parser {

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

struct Result {
	shared_ptr<TokenStream> tokens;
	vector<Report> reports;
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