#pragma once

namespace evm { class VM; }

namespace tt
{

enum MathOpCode
{
	OP_VEC3_STORE = 32,
	OP_VEC3_PRINT,

	OP_MATRIX_CREATE,
	OP_MATRIX_ROTATE,
	OP_MATRIX_TRANSLATE,
};

class MathOpCodeImpl
{
public:
	static void OpCodeInit(evm::VM* vm);

private:
	static void Vec3Create(evm::VM* vm);
	static void Vec3Print(evm::VM* vm);

	static void MatrixCreate(evm::VM* vm);
	static void MatrixRotate(evm::VM* vm);
	static void MatrixTranslate(evm::VM* vm);

}; // MathOpCodeImpl

}