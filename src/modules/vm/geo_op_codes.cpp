#include "geo_op_codes.h"

#include <SM_Vector.h>
#include <SM_Plane.h>
#include <polymesh3/Polytope.h>
#include <easyvm/VM.h>
#include <easyvm/VMHelper.h>
#include <easyvm/Value.h>

namespace tt
{

void GeoOpCodeImpl::OpCodeInit(evm::VM* vm)
{
	vm->RegistOperator(OP_VEC3_STORE, Vec3Create);
	vm->RegistOperator(OP_VEC3_PRINT, Vec3Print);
	vm->RegistOperator(OP_CREATE_PLANE, CreatePlane);
	vm->RegistOperator(OP_CREATE_POLYFACE, CreatePolyFace);
	vm->RegistOperator(OP_CREATE_POLYTOPE, CreatePolytope);
}

void GeoOpCodeImpl::Vec3Create(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();

	auto v3 = std::make_shared<sm::vec3>();
	v3->x = evm::VMHelper::ReadData<float>(vm);
	v3->y = evm::VMHelper::ReadData<float>(vm);
	v3->z = evm::VMHelper::ReadData<float>(vm);

	evm::Value v;
	v.type = evm::ValueType::HANDLE;
	v.as.handle = new evm::Handle<sm::vec3>(v3);

	vm->SetRegister(reg, v);
}

void GeoOpCodeImpl::Vec3Print(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();
	auto vec = evm::VMHelper::GetHandler<sm::vec3>(vm, reg);
	if (vec) {
		printf("%f, %f, %f", vec->x, vec->y, vec->z);
	}
}

void GeoOpCodeImpl::CreatePlane(evm::VM* vm)
{
	uint8_t dst_reg = vm->NextByte();

	uint8_t p0_reg = vm->NextByte();
	auto p0 = evm::VMHelper::GetHandler<sm::vec3>(vm, p0_reg);

	uint8_t p1_reg = vm->NextByte();
	auto p1 = evm::VMHelper::GetHandler<sm::vec3>(vm, p1_reg);

	uint8_t p2_reg = vm->NextByte();
	auto p2 = evm::VMHelper::GetHandler<sm::vec3>(vm, p2_reg);

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

	auto plane = evm::VMHelper::GetHandler<sm::Plane>(vm, plane_reg);

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

	auto faces = evm::VMHelper::GetHandler<std::vector<evm::Value>>(vm, src_reg);

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

}