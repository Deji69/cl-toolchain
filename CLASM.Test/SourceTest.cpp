#include "catch.hpp"
#include <CLASM/Source.h>

using namespace CLARA;
using namespace CLARA::CLASM;

TEST_CASE("Source object is constructed", "[Source]")
{
	Source source{"test", "nop"};
	REQUIRE(source.getName() == "test");
	REQUIRE(source.getCode() == "nop");
}

TEST_CASE("Sorce object is constructed with line info", "[Source]")
{
	{
		Source source{"test", "lets"};
		REQUIRE(source.getNumLines() == 1u);
		auto& ln = source.getLineInfo(0);
		REQUIRE(ln.number == 1);
		REQUIRE(ln.length == 4);
		REQUIRE(ln.offset == 0);
		REQUIRE(ln.charLength == 4);
	}

	Source source{"test", "lets\ncount\nsómé\n§linẽs"};
	REQUIRE(source.getNumLines() == 4);
	auto& lines = source.getLineInfos();
	REQUIRE(lines[0].number == 1);
	REQUIRE(lines[0].length == 4);
	REQUIRE(lines[0].offset == 0);
	REQUIRE(lines[0].charLength == 4);
	REQUIRE(lines[1].number == 2);
	REQUIRE(lines[1].length == 5);
	REQUIRE(lines[1].offset == 5);
	REQUIRE(lines[1].charLength == 5);
	REQUIRE(lines[2].number == 3);
	REQUIRE(lines[2].length == 6);
	REQUIRE(lines[2].offset == 11);
	REQUIRE(lines[2].charLength == 4);
	REQUIRE(lines[3].number == 4);
	REQUIRE(lines[3].length == 9);
	REQUIRE(lines[3].offset == 18);
	REQUIRE(lines[3].charLength == 6);
}

TEST_CASE("can get token", "[Source]")
{
	Source source1{"test", "hello"};
	REQUIRE(source1.getToken(0, 4).text == "hell");
	REQUIRE(source1.getToken(1, 3).text == "ell");
	REQUIRE(source1.getToken(1, 9).text == "ello");
	REQUIRE(source1.getToken(0, 0).text == "");
	REQUIRE_THROWS_AS(source1.getToken(5, 5), std::out_of_range);
}

TEST_CASE("can get token by whitespace delimiting", "[Source]")
{
	Source source1{"test", "hello world"};
	REQUIRE(source1.getToken(0).text == "hello");
	REQUIRE(source1.getToken(4).text == "o");
	REQUIRE(source1.getToken(5).text == "world");
	REQUIRE(source1.getToken(10).text == "d");
	REQUIRE_THROWS_AS(source1.getToken(11), std::out_of_range);
}

TEST_CASE("can get line index via offset", "[Source]")
{
	{
		Source source{"test", ""};
		REQUIRE(source.getLineIndexByOffset(4) == 0);
		REQUIRE(source.getLineIndexByOffset(0) == 0);
	}

	Source source{"test", "lets\ncóunt\n§ome\nlinẽs"};
	REQUIRE(source.getLineIndexByOffset(0) == 0);
	REQUIRE(source.getLineIndexByOffset(4) == 0);
	REQUIRE(source.getLineIndexByOffset(5) == 1);
	REQUIRE(source.getLineIndexByOffset(11) == 1);
	REQUIRE(source.getLineIndexByOffset(12) == 2);
	REQUIRE(source.getLineIndexByOffset(17) == 2);
	REQUIRE(source.getLineIndexByOffset(18) == 3);
	REQUIRE(source.getLineIndexByOffset(99) == 3);
}

TEST_CASE("can get column via offset", "[Source]")
{
	Source source{"test", "This line ɥás… § unicode\n⅒\ntest"};
	REQUIRE(source.getColumnByOffset(4) == 5);
	REQUIRE(source.getColumnByOffset(15) == 14);
	REQUIRE(source.getColumnByOffset(21) == 17);
	REQUIRE(source.getColumnByOffset(30) == 1);
	REQUIRE(source.getColumnByOffset(33) == 2);
	REQUIRE(source.getColumnByOffset(34) == 1);
	REQUIRE(source.getColumnByOffset(38) == 5);
}