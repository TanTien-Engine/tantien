#pragma once

namespace evm { class VM; }

namespace tt
{

enum GeoOpCode
{
	OP_CREATE_POLYFACE = 96,// plane
	OP_CREATE_POLYTOPE,		// faces
	OP_CREATE_POLYFACE_2,	// border, holes
	OP_CREATE_POLYTOPE_2,	// points, faces

	OP_POLYTOPE_SUBTRACT,
	OP_POLYTOPE_EXTRUDE,
	OP_POLYTOPE_CLIP,
	OP_POLYTOPE_SET_DIRTY,

	OP_POLYPOINT_SELECT,
	OP_POLYFACE_SELECT,

	OP_TRANSFORM,

	OP_POLY_COPY_FROM_MEM,
};

class GeoOpCodeImpl
{
public:
	static void OpCodeInit(evm::VM* vm);

private:
	static void CreatePolyFace(evm::VM* vm);
	static void CreatePolytope(evm::VM* vm);
	static void CreatePolyFace2(evm::VM* vm);
	static void CreatePolytope2(evm::VM* vm);

	static void PolytopeSubtract(evm::VM* vm);
	static void PolytopeExtrude(evm::VM* vm);
	static void PolytopeClip(evm::VM* vm);
	static void PolytopeSetDirty(evm::VM* vm);

	static void PolyPointSelect(evm::VM* vm);
	static void PolyFaceSelect(evm::VM* vm);

	static void Transform(evm::VM* vm);

	static void PolyCopyFromMem(evm::VM* vm);

}; // GeoOpCodeImpl

}