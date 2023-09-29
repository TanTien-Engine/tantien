#include "geo_opcodes.h"
#include "modules/geometry/PolytopeAlgos.h"

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

void GeoOpCodeImpl::CreatePolyFace2(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_border = vm->NextByte();
	auto v_border = evm::VMHelper::GetRegHandler<std::vector<evm::Value>>(vm, r_border);

	// todo: holes
	uint8_t r_holes = vm->NextByte();

	evm::Value val;
	val.type = evm::ValueType::HANDLE;
	val.as.handle = nullptr;

	if (!v_border)
	{
		vm->SetRegister(r_dst, val);
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
	v.type = evm::ValueType::HANDLE;
	v.as.handle = new evm::Handle<pm3::Polytope::Face>(face);

	vm->SetRegister(r_dst, v);
}

void GeoOpCodeImpl::CreatePolytope2(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_points = vm->NextByte();
	auto v_points = evm::VMHelper::GetRegHandler<std::vector<evm::Value>>(vm, r_points);

	uint8_t r_faces = vm->NextByte();
	auto v_faces = evm::VMHelper::GetRegHandler<std::vector<evm::Value>>(vm, r_faces);

	evm::Value val;
	val.type = evm::ValueType::HANDLE;
	val.as.handle = nullptr;

	if (!v_points || !v_faces)
	{
		vm->SetRegister(r_dst, val);
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

void GeoOpCodeImpl::PolytopeSubtract(evm::VM* vm)
{
	uint8_t reg_dst = vm->NextByte();

	uint8_t reg_base = vm->NextByte();
	auto base = evm::VMHelper::GetRegHandler<pm3::Polytope>(vm, reg_base);
	if (!base) {
		return;
	}

	uint8_t reg_tool = vm->NextByte();
	auto tool = evm::VMHelper::GetRegHandler<pm3::Polytope>(vm, reg_tool);
	if (!tool) {
		return;
	}

	auto base_topo = base->GetTopoPoly();
	auto tool_topo = tool->GetTopoPoly();
	if (!base_topo || !tool_topo) {
		return;
	}

	auto polytopes = base_topo->Subtract(*tool_topo);
	if (polytopes.empty()) {
		return;
	}

	if (polytopes.size() == 1)
	{
		auto poly = std::make_shared<pm3::Polytope>(polytopes[0]);

		evm::Value v;
		v.type = evm::ValueType::HANDLE;
		v.as.handle = new evm::Handle<pm3::Polytope>(poly);

		vm->SetRegister(reg_dst, v);
	}
	else
	{
		auto list = std::make_shared<std::vector<evm::Value>>();

		for (auto src : polytopes)
		{
			auto dst = std::make_shared<pm3::Polytope>(src);

			evm::Value v;
			v.type = evm::ValueType::HANDLE;
			v.as.handle = new evm::Handle<pm3::Polytope>(dst);

			list->push_back(v);
		}

		evm::Value v;
		v.type = evm::ValueType::HANDLE;
		v.as.handle = new evm::Handle<std::vector<evm::Value>>(list);

		vm->SetRegister(reg_dst, v);
	}
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