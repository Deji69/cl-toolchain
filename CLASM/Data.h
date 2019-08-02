#pragma once
#include <CLASM/Common.h>

namespace CLARA::CLASM {

enum class DataType {
	String,
};

struct ImportFile {
	fs::path path;
};

using Data = variant<
	string, ImportFile
>;

struct Data {

};

}