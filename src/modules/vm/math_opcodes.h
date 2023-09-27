#pragma once

namespace evm { class VM; }

namespace tt
{

enum MathOpCode
{
	OP_VEC3_STORE = 32,
	OP_VEC3_PRINT,
};

class MathOpCodeImpl
{
public:
	static void OpCodeInit(evm::VM* vm);

private:
	static void Vec3Create(evm::VM* vm);
	static void Vec3Print(evm::VM* vm);

}; // MathOpCodeImpl

}