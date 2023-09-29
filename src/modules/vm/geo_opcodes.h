#pragma once

namespace evm { class VM; }

namespace tt
{

enum GeoOpCode
{
	OP_CREATE_PLANE = 96,
	OP_CREATE_POLYFACE,		// plane
	OP_CREATE_POLYTOPE,		// faces
	OP_CREATE_POLYFACE_2,	// border, holes
	OP_CREATE_POLYTOPE_2,	// points, faces

	OP_POLYTOPE_TRANSFORM,
	OP_POLYTOPE_SUBTRACT,
	OP_POLYTOPE_EXTRUDE,
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
	static void CreatePolyFace2(evm::VM* vm);
	static void CreatePolytope2(evm::VM* vm);

	static void PolytopeTransform(evm::VM* vm);
	static void PolytopeSubtract(evm::VM* vm);
	static void PolytopeExtrude(evm::VM* vm);

}; // GeoOpCodeImpl

}