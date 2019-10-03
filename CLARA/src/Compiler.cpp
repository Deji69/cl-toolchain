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

	auto write(const uint8_t* begin, const uint8_t* end)
	{
		output.write(begin, end);
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

	auto write64(uint64 val)
	{
		output.write64(val);
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

	auto compileSegment(const Parser::SegmentInfo& segment)
	{
		if (!segment.tokens) return;
		for (auto it = segment.tokens->begin(); it != segment.tokens->end(); ++it) {
			std::visit([&](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;

				if constexpr (std::is_arithmetic_v<T>) {
					auto arr = encodeBytes(arg);
					write(reinterpret_cast<uint8_t*>(&arr[0]), reinterpret_cast<uint8_t*>(&arr[arr.size()]));
				}
				else if constexpr (std::is_same_v<T, std::string>) {
					write(reinterpret_cast<const uint8_t*>(&arg[0]), reinterpret_cast<const uint8_t*>(&arg[arg.size()]));
				}
				else if constexpr (std::is_same_v<T, Instruction::Type>) {
					write8(static_cast<uint8>(arg));
				}
				else if constexpr (std::is_same_v<T, const Label*>) {
					arg->offset = offset;
				}
				else if constexpr (std::is_same_v<T, LabelRef>) {
					write32(arg.label->offset);
				}
				else if constexpr (
					!std::is_same_v<T, monostate> &&
					!std::is_same_v<T, Segment::Type> &&
					!std::is_same_v<T, Keyword::Type> &&
					!std::is_same_v<T, Mnemonic::Type> &&
					!std::is_same_v<T, DataType::Type>
				) {
					static_assert(always_false<T>::value, "non-exhaustive visitor!");
				}
			}, it->annotation);
		}
	}
};

auto compile(const Options& opts, const Parser::ParseInfo& parsed, IBinaryOutput& out)->Result
{
	CompilerContext ctx{opts, out, parsed};
	for (auto& segment : parsed.segments) {
		ctx.compileSegment(segment);
	}
	return Result{};
}

}