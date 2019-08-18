#pragma once
#include <CLARA/Common/Imports.h>
#include <CLARA/IBinaryOutput.h>
#include <CLARA/Assembly.h>
#include <CLARA/Parser.h>
#include <CLARA/Reporter.h>

namespace CLARA::CLASM {

class Compiler {
public:
	Compiler(ReporterFunc = nullptr);

	auto compile(const TokenStream& tokens, IBinaryOutput& out)->void;

private:
	Reporter report;
};

}