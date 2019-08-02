#pragma once
#include <CLASM/Token.h>

namespace CLARA::CLASM {

struct Label {
	string name;
	const Token& definition;
};

struct LabelNameHash {
	auto operator()(const Label& i) const
	{
		return std::hash<string>()(i.name);
	}
};

struct LabelNameEq {
	auto operator()(const Label& i1, const Label& i2) const
	{
		return i1.name == i2.name;
	}
};

}