#include <CLARA/pch.h>
#include <CLARA/Assembly.h>
#include <CLARA/Compiler.h>

using namespace CLARA;
using namespace CLARA::CLASM;

namespace CLARA::CLASM::Compiler {

using TokenIterator = vector<Token>::const_iterator;

struct CompilerContext {
	const Options& options;
	const Reporter& report;
	IBinaryOutput& output;
	const TokenStream& tokenStream;
	TokenIterator it;
	Segment::Type segment = Segment::MAX;

	CompilerContext(const Options& opts, IBinaryOutput& out, const TokenStream& ts) :
		options(opts), report(opts.reporter), output(out), tokenStream(ts)
	{
		it = tokenStream.all().begin();
	}

	auto compileInstruction()
	{}
};

auto compile(const Options& opts, const TokenStream& ts, IBinaryOutput& out)->Result
{
	CompilerContext ctx{opts, out, ts};

	while (ctx.it != ts.all().end()) {
		switch (ctx.it->type) {
		case TokenType::Segment:
			if (auto segment = Segment::fromName(ctx.it->text.substr(1)); segment != Segment::MAX) {
				ctx.segment = segment;
			}
			else {
				ctx.report.error("Invalid segment name"sv);
			}
			break;

		case TokenType::Instruction:
			//ctx.compileInstruction(ctx);
			break;

		default:
			ctx.report.error("Unexpected token type");
			break;
		}
		++ctx.it;
	}

	out.write8(Instruction::NOP);
	return Result{};
}

}