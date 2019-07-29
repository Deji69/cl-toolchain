#pragma once
#include <CLASM/Common/Imports.h>
#include <CLASM/IBinaryOutput.h>
#include <CLASM/Assembly.h>
#include <CLASM/Parser.h>
#include <CLASM/Reporter.h>

namespace CLARA::CLASM {

class Compiler {
public:
	Compiler(ReporterFunc = nullptr);

	auto compile(const TokenStream& tokens, IBinaryOutput& out)->void;

private:
	Reporter report;
};

}