#pragma once
#include <CLASM/Common.h>

namespace CLARA::CLASM {

enum class OperandType {
	IMM8, IMM16, IMM32, IMM64,	//< immediate value
	LV8, LV16, LV32,			//< local var index
	V16, V32,					//< global var index
	S32,						//< string index
	REL32,						//< relative script offset
};

struct InstructionOverload;

struct InstructionOperand {
	vector<OperandType> types;
	bool variadic = false;

	InstructionOperand(vector<OperandType> types, bool variadic = false) :
		types(types), variadic(variadic)
	{ }
};

class Segment {
public:
	enum Type {
		None, Data, String, Code, MAX
	};

public:
	static auto fromName(string_view)->optional<Type>;
};

class Keyword {
public:
	enum Type {
		Global, Extern, Import, Include
	};

public:
	static auto fromName(const string&)->optional<Type>;
};

class Mnemonic {
public:
	enum Type : uint {
		PUSH, PUSHA, POP, DUP, JMP, CALL,
		MAX
	};

public:
	static auto fromName(const string&)->optional<Type>;
	static auto getOverloads(Type type)->const vector<InstructionOverload>&;
};

class Instruction {
public:
	enum Type : uint8_t {
		// Misc
		NOP, BREAK, THROW,
		// Stack Manipulation
		PUSHN, PUSHB, PUSHW, PUSHD, PUSHQ, PUSHF, PUSHQF,
		PUSHAB, PUSHAW, PUSHAD, PUSHAQ, PUSHAF, PUSHAQF, PUSHS,
		POP, POPLN, POPL, POPLE, POPV, POPVE,
		SWAP,
		DUP, DUPE,
		// Variable Access
		LOCAL, GLOBAL, ARRAY,
		// Arithimetic/Bitwise/Conversion Operations
		EXF,
		INC, DEC,
		ADD, SUB, MUL, DIV, MOD,
		NOT, AND, OR, XOR,
		SHL, SHR,
		NEG,
		TOI, TOF,
		// Comparison
		CMPNN, CMPE, CMPNE, CMPGE, CMPLE, CMPG, CMPL,
		IF, EVAL,
		// Branching
		JT, JNT, JMP, JMPD,
		SWITCH, RSWITCH,
		// Functions
		CALL, CALLD,
		ENTER, RET,
		// External Read / Write
		READ, WRITE, COPY, FILL, COMP,
		// External Calling
		NATIVE, CMD, CDECL, STDC, THISC, FASTC,
		//
		MAX
	};

public:
	static auto fromName(const string& sv)->optional<Type>;
	static auto getOperands(Type type)->const vector<InstructionOperand>&;
};

struct InstructionOverload {
	Instruction::Type insn;
	vector<OperandType> params;
};

}

template<>
inline auto CLARA::to_string(CLASM::OperandType type)
{
	switch (type) {
	case CLASM::OperandType::IMM8:
		return "immediate 8-bit value"s;
	case CLASM::OperandType::IMM16:
		return "immediate 16-bit value"s;
	case CLASM::OperandType::IMM32:
		return "immediate 32-bit value"s;
	case CLASM::OperandType::IMM64:
		return "immediate 64-bit value"s;
	case CLASM::OperandType::LV8:
	case CLASM::OperandType::LV16:
	case CLASM::OperandType::LV32:
		return "local variable"s;
	case CLASM::OperandType::V16:
	case CLASM::OperandType::V32:
		return "global variable"s;
	case CLASM::OperandType::S32:
		return "string"s;
	case CLASM::OperandType::REL32:
		return "32-bit offset"s;
	}

	throw std::invalid_argument("Unhandled operand type name");
}