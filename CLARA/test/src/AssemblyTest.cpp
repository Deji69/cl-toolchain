#include "catch.hpp"
#include <CLARA/Assembly.h>

using namespace CLARA;
using namespace CLARA::CLASM;

TEST_CASE("Get keyword by name", "[Assembly]")
{
	auto kw = Keyword::fromName("extern");
	REQUIRE(kw.has_value());
	REQUIRE(*kw == Keyword::Extern);
	kw = Keyword::fromName("global");
	REQUIRE(kw.has_value());
	REQUIRE(*kw == Keyword::Global);
	kw = Keyword::fromName("include_once");
	REQUIRE(!kw.has_value());
}

TEST_CASE("Get mnemonic by name", "[Assembly]")
{
	auto mn = Mnemonic::fromName("call");
	REQUIRE(mn);
	REQUIRE(*mn == Mnemonic::CALL);
	mn = Mnemonic::fromName("pusha");
	REQUIRE(mn);
	REQUIRE(*mn == Mnemonic::PUSHA);
	mn = Mnemonic::fromName("pushb");
	REQUIRE(!mn);
}

TEST_CASE("Get instruction by name", "[Assembly]")
{
	auto res = Instruction::fromName("add");
	REQUIRE(res);
	REQUIRE(*res == Instruction::ADD);
	res = Instruction::fromName("abb");
	REQUIRE(!res);
}

TEST_CASE("Get instruction overloads", "[Assembly]")
{
	auto& res = Mnemonic::getOverloads(Mnemonic::CALL);
	REQUIRE(res.size() == size_t{2});
	REQUIRE(res[0].insn == Instruction::CALL);
	REQUIRE(res[1].insn== Instruction::CALLD);
	REQUIRE(res[1].params[0] == OperandType::REL32);
}