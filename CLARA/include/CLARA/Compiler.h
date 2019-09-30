#pragma once
#include <CLARA/Assembly.h>
#include <CLARA/Common.h>
#include <CLARA/Diagnostic.h>
#include <CLARA/IBinaryOutput.h>
#include <CLARA/Parser.h>
#include <CLARA/Reporter.h>

namespace CLARA::CLASM::Compiler {

struct Options {
	Reporter reporter;
	bool errorReporting = true;
	bool testForceCompilation = false;
};

struct Result {
	
};

auto compile(const Options& options, const TokenStream& tokens, IBinaryOutput& out)->Result;

}