#pragma once

namespace evm { class VM; }

namespace tt
{

enum StlOpCode
{
	OP_VECTOR_CREATE = 32,
	OP_VECTOR_ADD,
};

class StlOpCodeImpl
{
public:
	static void OpCodeInit(evm::VM* vm);

private:
	static void VectorCreate(evm::VM* vm);
	static void VectorAdd(evm::VM* vm);

}; // StlOpCodeImpl

}