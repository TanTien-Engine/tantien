#pragma once

namespace evm { class VM; }

namespace tt
{

enum MathOpCode
{
	OP_VEC2_CREATE_I = 32,

	OP_VEC3_CREATE_R,
	OP_VEC3_CREATE_I,
	OP_VEC3_PRINT,
	OP_VEC3_ADD,
	OP_VEC3_SUB,
	OP_VEC3_TRANSFORM,

	OP_VEC4_CREATE_I,

	OP_MATRIX_CREATE,
	OP_MATRIX_ROTATE,
	OP_MATRIX_TRANSLATE,

	OP_GET_X,
	OP_GET_Y,
	OP_GET_Z,
	OP_GET_W,

	OP_CREATE_PLANE,
	OP_CREATE_PLANE_2,
	OP_CREATE_CUBE,

	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,

	OP_NEGATE,
};

class MathOpCodeImpl
{
public:
	static void OpCodeInit(evm::VM* vm);

private:
	static void Vec2CreateI(evm::VM* vm);

	static void Vec3CreateR(evm::VM* vm);
	static void Vec3CreateI(evm::VM* vm);
	static void Vec3Print(evm::VM* vm);
	static void Vec3Add(evm::VM* vm);
	static void Vec3Sub(evm::VM* vm);
	static void Vec3Transform(evm::VM* vm);

	static void Vec4CreateI(evm::VM* vm);

	static void MatrixCreate(evm::VM* vm);
	static void MatrixRotate(evm::VM* vm);
	static void MatrixTranslate(evm::VM* vm);

	static void GetX(evm::VM* vm);
	static void GetY(evm::VM* vm);
	static void GetZ(evm::VM* vm);
	static void GetW(evm::VM* vm);

	static void CreatePlane(evm::VM* vm);
	static void CreatePlane2(evm::VM* vm);
	static void CreateCube(evm::VM* vm);

	static void Add(evm::VM* vm);
	static void Sub(evm::VM* vm);
	static void Mul(evm::VM* vm);
	static void Div(evm::VM* vm);

	static void Negate(evm::VM* vm);

}; // MathOpCodeImpl

}