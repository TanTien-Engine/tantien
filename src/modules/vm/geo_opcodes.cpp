#include "geo_opcodes.h"
#include "ValueType.h"
#include "modules/geometry/PolytopeAlgos.h"
#include "modules/vm/VMHelper.h"

#include <SM_Matrix.h>
#include <polymesh3/Polytope.h>
#include <easyvm/VM.h>
#include <easyvm/VMHelper.h>
#include <easyvm/Value.h>

#include <stdexcept>

namespace tt
{

void GeoOpCodeImpl::OpCodeInit(evm::VM* vm)
{
	vm->RegistOperator(OP_CREATE_PLANE, CreatePlane);
	vm->RegistOperator(OP_CREATE_PLANE_2, CreatePlane2);
	vm->RegistOperator(OP_CREATE_POLYFACE, CreatePolyFace);
	vm->RegistOperator(OP_CREATE_POLYTOPE, CreatePolytope);
	vm->RegistOperator(OP_CREATE_POLYFACE_2, CreatePolyFace2);
	vm->RegistOperator(OP_CREATE_POLYTOPE_2, CreatePolytope2);

	vm->RegistOperator(OP_POLYTOPE_TRANSFORM, PolytopeTransform);
	vm->RegistOperator(OP_POLYTOPE_SUBTRACT, PolytopeSubtract);
	vm->RegistOperator(OP_POLYTOPE_EXTRUDE, PolytopeExtrude);
	vm->RegistOperator(OP_POLYTOPE_CLIP, PolytopeClip);

	vm->RegistOperator(OP_TRANSFORM_UNKNOWN, TransformUnknown);
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
	v.type = tt::ValueType::V_PLANE;
	v.as.handle = new evm::Handle<sm::Plane>(plane);

	vm->SetRegister(r_dst, v);
}

void GeoOpCodeImpl::CreatePlane2(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_ori = vm->NextByte();
	auto ori = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_ori);

	uint8_t r_dir = vm->NextByte();
	auto dir = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_dir);

	if (!ori || !dir)
	{
		vm->SetRegister(r_dst, evm::Value());
		return;
	}

	auto plane = std::make_shared<sm::Plane>();
	plane->Build(*ori, *dir);

	evm::Value v;
	v.type = tt::ValueType::V_PLANE;
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
	v.type = tt::ValueType::V_POLY_FACE;
	v.as.handle = new evm::Handle<pm3::Polytope::Face>(face);

	vm->SetRegister(r_dst, v);
}

void GeoOpCodeImpl::CreatePolytope(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src = vm->NextByte();

	auto faces = tt::VMHelper::GetRegArray(vm, r_src);
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
	v.type = tt::ValueType::V_POLY;
	v.as.handle = new evm::Handle<pm3::Polytope>(poly);

	vm->SetRegister(r_dst, v);
}

void GeoOpCodeImpl::CreatePolyFace2(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_border = vm->NextByte();
	// todo: holes
	uint8_t r_holes = vm->NextByte();

	auto v_border = tt::VMHelper::GetRegArray(vm, r_border);
	if (!v_border)
	{
		vm->SetRegister(r_dst, evm::Value());
		return;
	}

	std::vector<size_t> border;
	border.reserve(v_border->size());
	for (auto v : *v_border) {
		border.push_back(static_cast<size_t>(v.as.number));
	}

	auto face = std::make_shared<pm3::Polytope::Face>();
	face->border = border;

	evm::Value v;
	v.type = tt::ValueType::V_POLY_FACE;
	v.as.handle = new evm::Handle<pm3::Polytope::Face>(face);

	vm->SetRegister(r_dst, v);
}

void GeoOpCodeImpl::CreatePolytope2(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_points = vm->NextByte();
	auto v_points = tt::VMHelper::GetRegArray(vm, r_points);

	uint8_t r_faces = vm->NextByte();
	auto v_faces = tt::VMHelper::GetRegArray(vm, r_faces);

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
	v.type = tt::ValueType::V_POLY;
	v.as.handle = new evm::Handle<pm3::Polytope>(poly);

	vm->SetRegister(r_dst, v);
}

void GeoOpCodeImpl::PolytopeTransform(evm::VM* vm)
{
	uint8_t r_poly = vm->NextByte();
	auto polys = tt::VMHelper::LoadPolys(vm, r_poly);

	uint8_t r_mat = vm->NextByte();
	auto mat = evm::VMHelper::GetRegHandler<sm::mat4>(vm, r_mat);

	if (polys.empty() || !mat) {
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
	uint8_t r_dst  = vm->NextByte();
	uint8_t r_base = vm->NextByte();
	uint8_t r_tool = vm->NextByte();

	auto base = tt::VMHelper::LoadPolys(vm, r_base);
	if (base.empty()) {
		vm->SetRegister(r_dst, evm::Value());
		return;
	}

	auto tool = tt::VMHelper::LoadPolys(vm, r_tool);

	auto polys = PolytopeAlgos::Subtract(base, tool);
	VMHelper::StorePolys(vm, r_dst, polys);
}

void GeoOpCodeImpl::PolytopeExtrude(evm::VM* vm)
{
	uint8_t r_poly = vm->NextByte();
	uint8_t r_dist = vm->NextByte();

	auto poly = evm::VMHelper::GetRegHandler<pm3::Polytope>(vm, r_poly);
	if (!poly) {
		return;
	}

	auto dist = evm::VMHelper::GetRegNumber(vm, r_dist);

	PolytopeAlgos::Extrude(poly, static_cast<float>(dist));
}

void GeoOpCodeImpl::PolytopeClip(evm::VM* vm)
{
	uint8_t r_poly = vm->NextByte();
	auto polys = tt::VMHelper::LoadPolys(vm, r_poly);

	uint8_t r_plane = vm->NextByte();
	auto plane = evm::VMHelper::GetRegHandler<sm::Plane>(vm, r_plane);

	uint8_t keep = vm->NextByte();
	uint8_t seam = vm->NextByte();

	if (polys.empty() || !plane) {
		return;
	}

	for (auto poly : polys)
	{
		if (poly->GetTopoPoly()->Clip(*plane, he::Polyhedron::KeepType(keep), bool(seam))) {
			poly->SetTopoDirty();
		}
	}
}

void GeoOpCodeImpl::TransformUnknown(evm::VM* vm)
{
	uint8_t r_obj = vm->NextByte();

	uint8_t r_mat = vm->NextByte();
	auto mat = evm::VMHelper::GetRegHandler<sm::mat4>(vm, r_mat);
	if (!mat) {
		return;
	}

	evm::Value v_obj;
	if (!vm->GetRegister(r_obj, v_obj)) {
		return;
	}

	if (v_obj.type == tt::V_VEC3)
	{
		auto vec3 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_obj);
		if (!vec3) {
			return;
		}

		sm::vec3 ret = (*mat) * (*vec3);

		evm::Value v;
		v.type = tt::ValueType::V_VEC3;
		v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

		vm->SetRegister(r_obj, v);
	}
	else if (v_obj.type == tt::V_POLY || v_obj.type == tt::V_ARRAY)
	{
		auto polys = tt::VMHelper::LoadPolys(vm, r_obj);

		for (auto poly : polys)
		{
			auto& pts = poly->Points();
			for (auto& p : pts) {
				p->pos = *mat * p->pos;
			}

			poly->SetTopoDirty();
		}
	}
	else if (v_obj.type == tt::V_PLANE)
	{
		auto p = evm::VMHelper::GetRegHandler<sm::Plane>(vm, r_obj);
		if (!p) {
			return;
		}

		auto& norm = p->normal;

		sm::vec3 pos0;
		if (norm.x != 0)
		{
			pos0.x = -p->dist / norm.x;
			pos0.y = 0;
			pos0.z = 0;
		}
		else if (norm.y != 0)
		{
			pos0.x = 0;
			pos0.y = -p->dist / norm.y;
			pos0.z = 0;
		}
		else if (norm.z != 0)
		{
			pos0.x = 0;
			pos0.y = 0;
			pos0.z = -p->dist / norm.z;
		}

		auto pos1 = pos0 + norm;

		pos0 = *mat * pos0;
		pos1 = *mat * pos1;
		p->Build(pos1 - pos0, pos0);

		evm::Value v;
		v.type = tt::ValueType::V_PLANE;
		v.as.handle = new evm::Handle<sm::Plane>(std::make_shared<sm::Plane>(p->normal, p->dist));

		vm->SetRegister(r_obj, v);
	}
	else
	{
		throw std::runtime_error("Not Implemented!");
	}
}

}