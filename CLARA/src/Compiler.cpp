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
	const Parser::ParseInfo& parse;
	size_t offset = 0;

	CompilerContext(const Options& opts, IBinaryOutput& out, const Parser::ParseInfo& parse) :
		options(opts), report(opts.reporter), output(out), parse(parse)
	{
	}

	auto write8(uint8 val)
	{
		output.write8(val);
		offset += 1;
	}

	auto write16(uint16 val)
	{
		output.write16(val);
		offset += 2;
	}

	auto write32(uint32 val)
	{
		output.write32(val);
		offset += 4;
	}

	auto write64(uint32 val)
	{
		output.write32(val);
		offset += 8;
	}

	auto writeString(string_view sv)
	{
		output.write(sv);
		offset += sv.size();
	}

	auto writeInstruction(const Token& token)
	{
		write8(static_cast<uint8>(get<Instruction::Type>(token.annotation)));
	}

	auto compileCodeSegment(TokenStream::const_iterator it, TokenStream::const_iterator end)
	{
		for (; it != end; ++it) {
			std::visit(visitor{
				[](auto&&) {},
				[this](uint8 num) { write8(num); },
				[this](uint16 num) { write16(num); },
				[this](uint32 num) { write32(num); },
				[this](int8 num) { write8(static_cast<uint8>(num)); },
				[this](int16 num) { write16(static_cast<uint16>(num)); },
				[this](int32 num) { write32(static_cast<uint32>(num)); },
				[this](float num) { write32(*reinterpret_cast<uint32*>(&num)); },
				[this](double num) { write64(*reinterpret_cast<uint64*>(&num)); },
				[this](Instruction::Type id) { write8(static_cast<uint8>(id)); },
				[this](const Label* label) {
					label->setOffset(offset);
				},
				[this](LabelRef ref) {
					write32(ref.label->offset);
				}
			}, it->annotation);
		}
	}

	auto compileDataSegment(TokenStream::const_iterator it, TokenStream::const_iterator end)
	{
		for (; it != end; ++it) {
		}
	}
};

auto compile(const Options& opts, const Parser::ParseInfo& parsed, IBinaryOutput& out)->Result
{
	CompilerContext ctx{opts, out, parsed};

	for (auto& segment : parsed.segments[Segment::Data]) {
		ctx.compileDataSegment(segment.begin, segment.end);
	}

	for (auto& segment : parsed.segments[Segment::Code]) {
		ctx.compileCodeSegment(segment.begin, segment.end);
	}
	return Result{};
}

}