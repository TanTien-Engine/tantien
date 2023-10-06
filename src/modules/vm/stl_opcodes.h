#pragma once

namespace evm { class VM; }

namespace tt
{

enum StlOpCode
{
	OP_VECTOR_CREATE = 64,
	OP_VECTOR_ADD,
	OP_VECTOR_CONCAT,
};

class StlOpCodeImpl
{
public:
	static void OpCodeInit(evm::VM* vm);

private:
	static void VectorCreate(evm::VM* vm);
	static void VectorAdd(evm::VM* vm);
	static void VectorConcat(evm::VM* vm);

}; // StlOpCodeImpl

}