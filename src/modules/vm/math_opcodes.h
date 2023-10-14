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
	OP_VEC3_GET_X,
	OP_VEC3_GET_Y,
	OP_VEC3_GET_Z,

	OP_MATRIX_CREATE,
	OP_MATRIX_ROTATE,
	OP_MATRIX_TRANSLATE,

	OP_CREATE_PLANE,
	OP_CREATE_PLANE_2,
	OP_CREATE_CUBE,

	OP_MUL_UNKNOWN,
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
	static void Vec3GetX(evm::VM* vm);
	static void Vec3GetY(evm::VM* vm);
	static void Vec3GetZ(evm::VM* vm);

	static void MatrixCreate(evm::VM* vm);
	static void MatrixRotate(evm::VM* vm);
	static void MatrixTranslate(evm::VM* vm);

	static void CreatePlane(evm::VM* vm);
	static void CreatePlane2(evm::VM* vm);
	static void CreateCube(evm::VM* vm);

	static void MulUnknown(evm::VM* vm);

}; // MathOpCodeImpl

}