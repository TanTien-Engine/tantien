#pragma once

namespace evm { class VM; }

namespace tt
{

enum GeoOpCode
{
	OP_VEC3_STORE = 64,
	OP_VEC3_PRINT,

	OP_CREATE_PLANE,
	OP_CREATE_POLYFACE,
	OP_CREATE_POLYTOPE,
};

class GeoOpCodeImpl
{
public:
	static void OpCodeInit(evm::VM* vm);

private:
	static void Vec3Create(evm::VM* vm);
	static void Vec3Print(evm::VM* vm);

	static void CreatePlane(evm::VM* vm);
	static void CreatePolyFace(evm::VM* vm);
	static void CreatePolytope(evm::VM* vm);

}; // GeoOpCodeImpl

}