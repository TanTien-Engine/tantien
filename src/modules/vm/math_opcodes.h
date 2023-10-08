#pragma once

namespace evm { class VM; }

namespace tt
{

enum MathOpCode
{
	OP_VEC3_CREATE = 32,
	OP_VEC3_STORE,
	OP_VEC3_PRINT,
	OP_VEC3_ADD,
	OP_VEC3_SUB,
	OP_VEC3_TRANSFORM,

	OP_MATRIX_CREATE,
	OP_MATRIX_ROTATE,
	OP_MATRIX_TRANSLATE,
};

class MathOpCodeImpl
{
public:
	static void OpCodeInit(evm::VM* vm);

private:
	static void Vec3Create3(evm::VM* vm);
	static void Vec3Create(evm::VM* vm);
	static void Vec3Print(evm::VM* vm);
	static void Vec3Add(evm::VM* vm);
	static void Vec3Sub(evm::VM* vm);
	static void Vec3Transform(evm::VM* vm);

	static void MatrixCreate(evm::VM* vm);
	static void MatrixRotate(evm::VM* vm);
	static void MatrixTranslate(evm::VM* vm);

}; // MathOpCodeImpl

}