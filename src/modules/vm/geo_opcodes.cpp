#include "geo_opcodes.h"
#include "modules/geometry/PolytopeAlgos.h"
#include "modules/vm/VMHelper.h"

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
	vm->RegistOperator(OP_CREATE_POLYFACE_2, CreatePolyFace2);
	vm->RegistOperator(OP_CREATE_POLYTOPE_2, CreatePolytope2);

	vm->RegistOperator(OP_POLYTOPE_TRANSFORM, PolytopeTransform);
	vm->RegistOperator(OP_POLYTOPE_SUBTRACT, PolytopeSubtract);
	vm->RegistOperator(OP_POLYTOPE_EXTRUDE, PolytopeExtrude);
}

void GeoOpCodeImpl::CreatePlane(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_p0 = vm->NextByte();
	auto p0 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_p0);

	uint8_t r_p1 = vm->NextByte();
	auto p1 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_p1);

	uint8_t r_p2 = vm->NextByte();
	auto p2 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_p2);

	if (!p0 || !p1 || !p2) 
	{
		vm->SetRegister(r_dst, evm::Value());
		return;
	}

	auto plane = std::make_shared<sm::Plane>();
	plane->Build(*p0, *p1, *p2);

	evm::Value v;
	v.type = evm::ValueType::HANDLE;
	v.as.handle = new evm::Handle<sm::Plane>(plane);

	vm->SetRegister(r_dst, v);
}

void GeoOpCodeImpl::CreatePolyFace(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_plane = vm->NextByte();

	auto plane = evm::VMHelper::GetRegHandler<sm::Plane>(vm, r_plane);
	if (!plane)
	{
		vm->SetRegister(r_dst, evm::Value());
		return;
	}

	auto face = std::make_shared<pm3::Polytope::Face>();
	face->plane = *plane;

	evm::Value v;
	v.type = evm::ValueType::HANDLE;
	v.as.handle = new evm::Handle<pm3::Polytope::Face>(face);

	vm->SetRegister(r_dst, v);
}

void GeoOpCodeImpl::CreatePolytope(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src = vm->NextByte();

	auto faces = evm::VMHelper::GetRegArray(vm, r_src);
	if (!faces)
	{
		vm->SetRegister(r_dst, evm::Value());
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

	vm->SetRegister(r_dst, v);
}

void GeoOpCodeImpl::CreatePolyFace2(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_border = vm->NextByte();
	auto v_border = evm::VMHelper::GetRegArray(vm, r_border);
	if (!v_border)
	{
		vm->SetRegister(r_dst, evm::Value());
		return;
	}

	// todo: holes
	uint8_t r_holes = vm->NextByte();

	std::vector<size_t> border;
	border.reserve(v_border->size());
	for (auto v : *v_border) {
		border.push_back(static_cast<size_t>(v.as.number));
	}

	auto face = std::make_shared<pm3::Polytope::Face>();
	face->border = border;

	evm::Value v;
	v.type = evm::ValueType::HANDLE;
	v.as.handle = new evm::Handle<pm3::Polytope::Face>(face);

	vm->SetRegister(r_dst, v);
}

void GeoOpCodeImpl::CreatePolytope2(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_points = vm->NextByte();
	auto v_points = evm::VMHelper::GetRegArray(vm, r_points);

	uint8_t r_faces = vm->NextByte();
	auto v_faces = evm::VMHelper::GetRegArray(vm, r_faces);

	if (!v_points || !v_faces)
	{
		vm->SetRegister(r_dst, evm::Value());
		return;
	}

	std::vector<pm3::Polytope::PointPtr> points;
	for (auto v_p : *v_points)
	{
		auto pos = static_cast<evm::Handle<sm::vec3>*>(v_p.as.handle);
		points.push_back(std::make_shared<pm3::Polytope::Point>(*pos->obj));
	}

	std::vector<std::shared_ptr<pm3::Polytope::Face>> faces;
	for (auto v_f : *v_faces) 
	{
		auto face = static_cast<evm::Handle<pm3::Polytope::Face>*>(v_f.as.handle);
		faces.push_back(face->obj);
	}

	auto poly = std::make_shared<pm3::Polytope>(points, faces);

	evm::Value v;
	v.type = evm::ValueType::HANDLE;
	v.as.handle = new evm::Handle<pm3::Polytope>(poly);

	vm->SetRegister(r_dst, v);
}

void GeoOpCodeImpl::PolytopeTransform(evm::VM* vm)
{
	uint8_t r_poly = vm->NextByte();
	auto polys = tt::VMHelper::LoadPolys(vm, r_poly);
	if (polys.empty()) {
		return;
	}

	uint8_t r_mat = vm->NextByte();
	auto mat = evm::VMHelper::GetRegHandler<sm::mat4>(vm, r_mat);
	if (!mat) {
		return;
	}

	for (auto poly : polys)
	{
		auto& pts = poly->Points();
		for (auto& p : pts) {
			p->pos = *mat * p->pos;
		}

		poly->SetTopoDirty();
	}
}

void GeoOpCodeImpl::PolytopeSubtract(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_base = vm->NextByte();
	auto base = tt::VMHelper::LoadPolys(vm, r_base);
	if (base.empty()) {
		vm->SetRegister(r_dst, evm::Value());
		return;
	}

	uint8_t r_tool = vm->NextByte();
	auto tool = tt::VMHelper::LoadPolys(vm, r_tool);

	auto polys = PolytopeAlgos::Subtract(base, tool);
	VMHelper::StorePolys(vm, r_dst, polys);
}

void GeoOpCodeImpl::PolytopeExtrude(evm::VM* vm)
{
	uint8_t r_poly = vm->NextByte();
	auto poly = evm::VMHelper::GetRegHandler<pm3::Polytope>(vm, r_poly);
	if (!poly) {
		return;
	}

	uint8_t r_dist = vm->NextByte();
	auto dist = evm::VMHelper::GetRegNumber(vm, r_dist);

	PolytopeAlgos::Extrude(poly, static_cast<float>(dist));
}

}