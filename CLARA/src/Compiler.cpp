#include <CLARA/pch.h>
#include <CLARA/Assembly.h>
#include <CLARA/Compiler.h>

using namespace CLARA;
using namespace CLARA::CLASM;

using TokenIterator = vector<Token>::const_iterator;

struct CompilerContext {
	Reporter& report;
	IBinaryOutput& output;
	const TokenStream& tokenStream;
	TokenIterator it;
	Segment::Type segment = Segment::None;

	CompilerContext(Reporter& report, IBinaryOutput& out, const TokenStream& ts) :
		report(report), output(out), tokenStream(ts)
	{
		it = tokenStream.all().begin();
	}

	auto compileInstruction()
	{ }
};

Compiler::Compiler(ReporterFunc reporter): report(reporter)
{ }

auto Compiler::compile(const TokenStream& ts, IBinaryOutput& out)->void
{
	CompilerContext ctx{report, out, ts};

	while (ctx.it != ts.all().end()) {
		switch (ctx.it->type) {
		case TokenType::Segment:
			if (auto segment = Segment::fromName(ctx.it->text.substr(1))) {
				ctx.segment = *segment;
			}
			else {
				report.error("Invalid segment name"sv);
			}
			break;

		case TokenType::Instruction:
			ctx.compileInstruction();
			break;

		default:
			report.error("Unexpected token type");
			break;
		}
		++ctx.it;
	}

	out.writeU8(Instruction::NOP);
}
