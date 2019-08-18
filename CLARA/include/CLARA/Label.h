#pragma once
#include <CLARA/Token.h>

namespace CLARA::CLASM {

struct Label {
	string name;
	const Token& definition;
	size_t offset = 0;
};

}