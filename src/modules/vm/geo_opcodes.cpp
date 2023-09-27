#include "geo_opcodes.h"

#include <SM_Matrix.h>
#include <polymesh3/Polytope.h>
#include <easyvm/VM.h>
#include <easyvm/VMHelper.h>
#include <easyvm/Value.h>

namespace tt
{

void GeoOpCodeImpl::OpCodeInit(evm::VM* vm)
{
	vm->RegistOperator(OP_CREATE_PLANE, CreatePlane);
	vm->RegistOperator(OP_CREATE_POLYFACE, CreatePolyFace);
	vm->RegistOperator(OP_CREATE_POLYTOPE, CreatePolytope);

	vm->RegistOperator(OP_POLYTOPE_TRANSFORM, PolytopeTransform);
}

void GeoOpCodeImpl::CreatePlane(evm::VM* vm)
{
	uint8_t dst_reg = vm->NextByte();

	uint8_t p0_reg = vm->NextByte();
	auto p0 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, p0_reg);

	uint8_t p1_reg = vm->NextByte();
	auto p1 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, p1_reg);

	uint8_t p2_reg = vm->NextByte();
	auto p2 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, p2_reg);

	evm::Value val;
	val.type = evm::ValueType::HANDLE;
	val.as.handle = nullptr;

	if (!p0 || !p1 || !p2) 
	{
		vm->SetRegister(dst_reg, val);
		return;
	}

	auto plane = std::make_shared<sm::Plane>();
	plane->Build(*p0, *p1, *p2);

	evm::Value v;
	v.type = evm::ValueType::HANDLE;
	v.as.handle = new evm::Handle<sm::Plane>(plane);

	vm->SetRegister(dst_reg, v);
}

void GeoOpCodeImpl::CreatePolyFace(evm::VM* vm)
{
	uint8_t dst_reg = vm->NextByte();

	uint8_t plane_reg = vm->NextByte();

	auto plane = evm::VMHelper::GetRegHandler<sm::Plane>(vm, plane_reg);

	evm::Value val;
	val.type = evm::ValueType::HANDLE;
	val.as.handle = nullptr;

	if (!plane)
	{
		vm->SetRegister(dst_reg, val);
		return;
	}

	auto face = std::make_shared<pm3::Polytope::Face>();
	face->plane = *plane;

	evm::Value v;
	v.type = evm::ValueType::HANDLE;
	v.as.handle = new evm::Handle<pm3::Polytope::Face>(face);

	vm->SetRegister(dst_reg, v);
}

void GeoOpCodeImpl::CreatePolytope(evm::VM* vm)
{
	uint8_t dst_reg = vm->NextByte();
	uint8_t src_reg = vm->NextByte();

	auto faces = evm::VMHelper::GetRegHandler<std::vector<evm::Value>>(vm, src_reg);

	evm::Value val;
	val.type = evm::ValueType::HANDLE;
	val.as.handle = nullptr;

	if (!faces)
	{
		vm->SetRegister(dst_reg, val);
		return;
	}

	std::vector<std::shared_ptr<pm3::Polytope::Face>> faces_vec;
	for (auto f : *faces) 
	{
		auto face = static_cast<evm::Handle<pm3::Polytope::Face>*>(f.as.handle);
		faces_vec.push_back(face->obj);
	}

	auto poly = std::make_shared<pm3::Polytope>(faces_vec);

	evm::Value v;
	v.type = evm::ValueType::HANDLE;
	v.as.handle = new evm::Handle<pm3::Polytope>(poly);

	vm->SetRegister(dst_reg, v);
}

void GeoOpCodeImpl::PolytopeTransform(evm::VM* vm)
{
	uint8_t reg_poly = vm->NextByte();
	auto poly = evm::VMHelper::GetRegHandler<pm3::Polytope>(vm, reg_poly);
	if (!poly) {
		return;
	}

	uint8_t reg_mat = vm->NextByte();
	auto mat = evm::VMHelper::GetRegHandler<sm::mat4>(vm, reg_mat);
	if (!mat) {
		return;
	}

	auto& pts = poly->Points();
	for (auto& p : pts) {
		p->pos = *mat * p->pos;
	}

	poly->SetTopoDirty();
}

}