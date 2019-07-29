#pragma once
#include <CLASM/Common.h>
#include <CLASM/TokenStream.h>
#include <CLASM/Assembly.h>
#include <CLASM/Reporter.h>
#include <CLASM/Diagnostic.h>
#include <CLASM/Source.h>

namespace CLARA::CLASM {

struct ParserReport {
	ReportType type;
	Source::Token token;
	Diagnosis diagnosis;

	ParserReport() = default;
	ParserReport(ReportType type, Source::Token token, Diagnosis&& diagnosis);

	static auto info(const Source::Token&, Diagnosis&&)->ParserReport;
	static auto warning(const Source::Token&, Diagnosis&&)->ParserReport;
	static auto error(const Source::Token&, Diagnosis&&)->ParserReport;
	static auto fatal(const Source::Token&, Diagnosis&&)->ParserReport;
};

struct ParserResult {
	shared_ptr<TokenStream> tokens;
	vector<ParserReport> reports;
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

struct ParserOptions {
	bool errorReporting = true;
};

class Parser {
public:
	Parser() = default;
	Parser(const ParserOptions& options);
	Parser(ReporterFunc reporter, const ParserOptions& options = ParserOptions{});

	auto tokenize(shared_ptr<const Source> source)->ParserResult;

private:
	ParserOptions options;
	Reporter reporter;
};

}