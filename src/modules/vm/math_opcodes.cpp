#include "math_opcodes.h"
#include "ValueType.h"

#include <SM_Vector.h>
#include <SM_Plane.h>
#include <SM_Matrix.h>
#include <SM_Cube.h>
#include <easyvm/VM.h>
#include <easyvm/VMHelper.h>
#include <easyvm/Value.h>

#include <stdexcept>

namespace tt
{

void MathOpCodeImpl::OpCodeInit(evm::VM* vm)
{
	vm->RegistOperator(OP_VEC2_CREATE_I, Vec2CreateI);

	vm->RegistOperator(OP_VEC3_CREATE_R, Vec3CreateR);
	vm->RegistOperator(OP_VEC3_CREATE_I, Vec3CreateI);
	vm->RegistOperator(OP_VEC3_PRINT, Vec3Print);
	vm->RegistOperator(OP_VEC3_ADD, Vec3Add);
	vm->RegistOperator(OP_VEC3_SUB, Vec3Sub);
	vm->RegistOperator(OP_VEC3_TRANSFORM, Vec3Transform);
	vm->RegistOperator(OP_VEC3_GET_X, Vec3GetX);
	vm->RegistOperator(OP_VEC3_GET_Y, Vec3GetY);
	vm->RegistOperator(OP_VEC3_GET_Z, Vec3GetZ);

	vm->RegistOperator(OP_VEC4_CREATE_I, Vec4CreateI);

	vm->RegistOperator(OP_MATRIX_CREATE, MatrixCreate);
	vm->RegistOperator(OP_MATRIX_ROTATE, MatrixRotate);
	vm->RegistOperator(OP_MATRIX_TRANSLATE, MatrixTranslate);

	vm->RegistOperator(OP_CREATE_PLANE, CreatePlane);
	vm->RegistOperator(OP_CREATE_PLANE_2, CreatePlane2);
	vm->RegistOperator(OP_CREATE_CUBE, CreateCube);

	vm->RegistOperator(OP_ADD, Add);
	vm->RegistOperator(OP_SUB, Sub);
	vm->RegistOperator(OP_MUL, Mul);
	vm->RegistOperator(OP_DIV, Div);
}

void MathOpCodeImpl::Vec2CreateI(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();

	auto v2 = std::make_shared<sm::vec2>();
	v2->x = evm::VMHelper::ReadData<float>(vm);
	v2->y = evm::VMHelper::ReadData<float>(vm);

	evm::Value v;
	v.type = tt::ValueType::V_VEC2;
	v.as.handle = new evm::Handle<sm::vec2>(v2);

	vm->SetRegister(reg, v);
}

void MathOpCodeImpl::Vec3CreateR(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_x = vm->NextByte();
	uint8_t r_y = vm->NextByte();
	uint8_t r_z = vm->NextByte();

	auto v3 = std::make_shared<sm::vec3>();
	if (r_x != 0xff) {
		v3->x = static_cast<float>(evm::VMHelper::GetRegNumber(vm, r_x));
	}
	if (r_y != 0xff) {
		v3->y = static_cast<float>(evm::VMHelper::GetRegNumber(vm, r_y));
	}
	if (r_z != 0xff) {
		v3->z = static_cast<float>(evm::VMHelper::GetRegNumber(vm, r_z));
	}

	evm::Value v;
	v.type = tt::ValueType::V_VEC3;
	v.as.handle = new evm::Handle<sm::vec3>(v3);

	vm->SetRegister(r_dst, v);
}

void MathOpCodeImpl::Vec3CreateI(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();

	auto v3 = std::make_shared<sm::vec3>();
	v3->x = evm::VMHelper::ReadData<float>(vm);
	v3->y = evm::VMHelper::ReadData<float>(vm);
	v3->z = evm::VMHelper::ReadData<float>(vm);

	evm::Value v;
	v.type = tt::ValueType::V_VEC3;
	v.as.handle = new evm::Handle<sm::vec3>(v3);

	vm->SetRegister(reg, v);
}

void MathOpCodeImpl::Vec3Print(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();
	auto vec = evm::VMHelper::GetRegHandler<sm::vec3>(vm, reg);
	if (vec) {
		printf("%f, %f, %f", vec->x, vec->y, vec->z);
	}
}

void MathOpCodeImpl::Vec3Add(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_src1 = vm->NextByte();
	uint8_t r_src2 = vm->NextByte();

	auto src1 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_src1);
	auto src2 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_src2);
	if (!src1 && !src2)
	{
		evm::Value v;
		vm->SetRegister(r_dst, v);
		return;
	}

	sm::vec3 ret;
	if (src1 && src2) {
		ret = *src1 + *src2;
	} else if (src1) {
		ret = *src1;
	} else {
		assert(src2);
		ret = *src2;
	}

	evm::Value v;
	v.type = tt::ValueType::V_VEC3;
	v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

	vm->SetRegister(r_dst, v);
}

void MathOpCodeImpl::Vec3Sub(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_src1 = vm->NextByte();
	uint8_t r_src2 = vm->NextByte();

	auto src1 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_src1);
	auto src2 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_src2);
	if (!src1 && !src2)
	{
		evm::Value v;
		vm->SetRegister(r_dst, v);
		return;
	}

	sm::vec3 ret;
	if (src1 && src2) {
		ret = *src1 - *src2;
	} else if (src1) {
		ret = *src1;
	} else {
		assert(src2);
		ret = -*src2;
	}

	evm::Value v;
	v.type = tt::ValueType::V_VEC3;
	v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

	vm->SetRegister(r_dst, v);
}

void MathOpCodeImpl::Vec3Transform(evm::VM* vm)
{
	uint8_t r_vec3 = vm->NextByte();
	uint8_t r_mat = vm->NextByte();

	auto vec3 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_vec3);
	if (!vec3) {
		return;
	}

	auto mat = evm::VMHelper::GetRegHandler<sm::mat4>(vm, r_mat);
	if (!mat) {
		return;
	}

	sm::vec3 ret = (*mat) * (*vec3);

	evm::Value v;
	v.type = tt::ValueType::V_VEC3;
	v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

	vm->SetRegister(r_vec3, v);
}

void MathOpCodeImpl::Vec3GetX(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src = vm->NextByte();

	auto vec3 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_src);
	if (!vec3) {
		return;
	}

	evm::Value val;
	val.type = evm::ValueType::V_NUMBER;
	val.as.number = vec3->x;

	vm->SetRegister(r_dst, val);
}

void MathOpCodeImpl::Vec3GetY(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src = vm->NextByte();

	auto vec3 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_src);
	if (!vec3) {
		return;
	}

	evm::Value val;
	val.type = evm::ValueType::V_NUMBER;
	val.as.number = vec3->y;

	vm->SetRegister(r_dst, val);
}

void MathOpCodeImpl::Vec3GetZ(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src = vm->NextByte();

	auto vec3 = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_src);
	if (!vec3) {
		return;
	}

	evm::Value val;
	val.type = evm::ValueType::V_NUMBER;
	val.as.number = vec3->z;

	vm->SetRegister(r_dst, val);
}

void MathOpCodeImpl::Vec4CreateI(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();

	auto v4 = std::make_shared<sm::vec4>();
	v4->x = evm::VMHelper::ReadData<float>(vm);
	v4->y = evm::VMHelper::ReadData<float>(vm);
	v4->z = evm::VMHelper::ReadData<float>(vm);
	v4->w = evm::VMHelper::ReadData<float>(vm);

	evm::Value v;
	v.type = tt::ValueType::V_VEC4;
	v.as.handle = new evm::Handle<sm::vec4>(v4);

	vm->SetRegister(reg, v);
}

void MathOpCodeImpl::MatrixCreate(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();

	auto mat = std::make_shared<sm::mat4>();

	evm::Value v;
	v.type = tt::ValueType::V_MAT4;
	v.as.handle = new evm::Handle<sm::mat4>(mat);

	vm->SetRegister(reg, v);
}

void MathOpCodeImpl::MatrixRotate(evm::VM* vm)
{
	uint8_t r_mat = vm->NextByte();
	uint8_t r_xyz = vm->NextByte();

	auto mat = evm::VMHelper::GetRegHandler<sm::mat4>(vm, r_mat);
	if (!mat) {
		return;
	}

	auto vec = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_xyz);
	if (!vec) {
		return;
	}

	mat->Rotate(vec->x, vec->y, vec->z);
}

void MathOpCodeImpl::MatrixTranslate(evm::VM* vm)
{
	uint8_t r_mat = vm->NextByte();
	uint8_t r_xyz = vm->NextByte();

	auto mat = evm::VMHelper::GetRegHandler<sm::mat4>(vm, r_mat);
	if (!mat) {
		return;
	}

	auto vec = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_xyz);
	if (!vec) {
		return;
	}

	mat->Translate(vec->x, vec->y, vec->z);
}

void MathOpCodeImpl::CreatePlane(evm::VM* vm)
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
		evm::Value v;
		vm->SetRegister(r_dst, v);
		return;
	}

	auto plane = std::make_shared<sm::Plane>();
	plane->Build(*p0, *p1, *p2);

	evm::Value v;
	v.type = tt::ValueType::V_PLANE;
	v.as.handle = new evm::Handle<sm::Plane>(plane);

	vm->SetRegister(r_dst, v);
}

void MathOpCodeImpl::CreatePlane2(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_ori = vm->NextByte();
	auto ori = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_ori);

	uint8_t r_dir = vm->NextByte();
	auto dir = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_dir);

	if (!ori || !dir)
	{
		evm::Value v;
		vm->SetRegister(r_dst, v);
		return;
	}

	auto plane = std::make_shared<sm::Plane>();
	plane->Build(*dir, *ori);

	evm::Value v;
	v.type = tt::ValueType::V_PLANE;
	v.as.handle = new evm::Handle<sm::Plane>(plane);

	vm->SetRegister(r_dst, v);
}

void MathOpCodeImpl::CreateCube(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();

	uint8_t r_min = vm->NextByte();
	auto min = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_min);

	uint8_t r_max = vm->NextByte();
	auto max = evm::VMHelper::GetRegHandler<sm::vec3>(vm, r_max);

	if (!min || !max)
	{
		evm::Value v;
		vm->SetRegister(r_dst, v);
		return;
	}

	auto cube = std::make_shared<sm::cube>(*min, *max);

	evm::Value v;
	v.type = tt::ValueType::V_CUBE;
	v.as.handle = new evm::Handle<sm::cube>(cube);

	vm->SetRegister(r_dst, v);
}

void MathOpCodeImpl::Add(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src1 = vm->NextByte();
	uint8_t r_src2 = vm->NextByte();

	bool b_src1 = r_src1 != 0xff;
	bool b_src2 = r_src2 != 0xff;
	if (!b_src1 && !b_src2)
	{
		evm::Value v;
		vm->SetRegister(r_dst, v);
		return;
	}
	else if (!b_src1 || !b_src2)
	{
		if (!b_src1) {
			vm->SetRegister(r_dst, vm->GetRegister(r_src2));
		} else {
			vm->SetRegister(r_dst, vm->GetRegister(r_src1));
		}
		return;
	}

	auto& src1 = vm->GetRegister(r_src1);
	auto& src2 = vm->GetRegister(r_src2);
	if (src1.type == evm::ValueType::V_NUMBER &&
		src2.type == evm::ValueType::V_NUMBER)
	{
		evm::Value val;
		val.type = evm::ValueType::V_NUMBER;
		val.as.number = src1.as.number + src2.as.number;

		vm->SetRegister(r_dst, val);
	}
	else if (src1.type == tt::ValueType::V_VEC2 &&
		     src2.type == tt::ValueType::V_VEC2)
	{
		sm::vec2 src1_v2 = *evm::VMHelper::GetHandleValue<sm::vec2>(src1);
		sm::vec2 src2_v2 = *evm::VMHelper::GetHandleValue<sm::vec2>(src2);

		sm::vec2 ret = src1_v2 + src2_v2;

		evm::Value v;
		v.type = tt::ValueType::V_VEC2;
		v.as.handle = new evm::Handle<sm::vec2>(std::make_shared<sm::vec2>(ret));

		vm->SetRegister(r_dst, v);
	}
	else if (src1.type == tt::ValueType::V_VEC3 &&
		     src2.type == tt::ValueType::V_VEC3)
	{
		sm::vec3 src1_v3 = *evm::VMHelper::GetHandleValue<sm::vec3>(src1);
		sm::vec3 src2_v3 = *evm::VMHelper::GetHandleValue<sm::vec3>(src2);

		sm::vec3 ret = src1_v3 + src2_v3;

		evm::Value v;
		v.type = tt::ValueType::V_VEC3;
		v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

		vm->SetRegister(r_dst, v);
	}
	else if (src1.type == tt::ValueType::V_VEC4 &&
		     src2.type == tt::ValueType::V_VEC4)
	{
		sm::vec4 src1_v4 = *evm::VMHelper::GetHandleValue<sm::vec4>(src1);
		sm::vec4 src2_v4 = *evm::VMHelper::GetHandleValue<sm::vec4>(src2);

		sm::vec4 ret = src1_v4 + src2_v4;

		evm::Value v;
		v.type = tt::ValueType::V_VEC4;
		v.as.handle = new evm::Handle<sm::vec4>(std::make_shared<sm::vec4>(ret));

		vm->SetRegister(r_dst, v);
	}
	else
	{
		throw std::runtime_error("Not Implemented!");
	}
}

void MathOpCodeImpl::Sub(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src1 = vm->NextByte();
	uint8_t r_src2 = vm->NextByte();

	bool b_src1 = r_src1 != 0xff;
	bool b_src2 = r_src2 != 0xff;
	if (!b_src1 && !b_src2)
	{
		evm::Value v;
		vm->SetRegister(r_dst, v);
		return;
	}
	else if (!b_src1 || !b_src2)
	{
		if (!b_src1) 
		{
			auto& src2 = vm->GetRegister(r_src2);

			evm::Value val;

			switch (src2.type)
			{
			case evm::ValueType::V_NUMBER:
			{
				val.type = evm::ValueType::V_NUMBER;
				val.as.number = -src2.as.number;
			}
				break;
			case tt::ValueType::V_VEC2:
			{
				sm::vec2 src2_v2 = *evm::VMHelper::GetHandleValue<sm::vec2>(src2);

				val.type = tt::ValueType::V_VEC2;
				val.as.handle = new evm::Handle<sm::vec2>(std::make_shared<sm::vec2>(-src2_v2));
			}
				break;
			case tt::ValueType::V_VEC3:
			{
				sm::vec3 src2_v3 = *evm::VMHelper::GetHandleValue<sm::vec3>(src2);

				val.type = tt::ValueType::V_VEC3;
				val.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(-src2_v3));
			}
				break;
			case tt::ValueType::V_VEC4:
			{
				sm::vec4 src2_v4 = *evm::VMHelper::GetHandleValue<sm::vec4>(src2);

				src2_v4.x = -src2_v4.x;
				src2_v4.y = -src2_v4.y;
				src2_v4.z = -src2_v4.z;
				src2_v4.w = -src2_v4.w;

				val.type = tt::ValueType::V_VEC4;
				val.as.handle = new evm::Handle<sm::vec4>(std::make_shared<sm::vec4>(src2_v4));
			}
				break;
			}

			vm->SetRegister(r_dst, val);
		} 
		else 
		{
			vm->SetRegister(r_dst, vm->GetRegister(r_src1));
		}
		return;
	}

	auto& src1 = vm->GetRegister(r_src1);
	auto& src2 = vm->GetRegister(r_src2);
	if (src1.type == evm::ValueType::V_NUMBER &&
		src2.type == evm::ValueType::V_NUMBER)
	{
		evm::Value val;
		val.type = evm::ValueType::V_NUMBER;
		val.as.number = src1.as.number - src2.as.number;

		vm->SetRegister(r_dst, val);
	}
	else if (src1.type == tt::ValueType::V_VEC2 &&
		     src2.type == tt::ValueType::V_VEC2)
	{
		sm::vec2 src1_v2 = *evm::VMHelper::GetHandleValue<sm::vec2>(src1);
		sm::vec2 src2_v2 = *evm::VMHelper::GetHandleValue<sm::vec2>(src2);

		sm::vec2 ret = src1_v2 - src2_v2;

		evm::Value v;
		v.type = tt::ValueType::V_VEC2;
		v.as.handle = new evm::Handle<sm::vec2>(std::make_shared<sm::vec2>(ret));

		vm->SetRegister(r_dst, v);
	}
	else if (src1.type == tt::ValueType::V_VEC3 &&
		     src2.type == tt::ValueType::V_VEC3)
	{
		sm::vec3 src1_v3 = *evm::VMHelper::GetHandleValue<sm::vec3>(src1);
		sm::vec3 src2_v3 = *evm::VMHelper::GetHandleValue<sm::vec3>(src2);

		sm::vec3 ret = src1_v3 - src2_v3;

		evm::Value v;
		v.type = tt::ValueType::V_VEC3;
		v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

		vm->SetRegister(r_dst, v);
	}
	else if (src1.type == tt::ValueType::V_VEC4 &&
		     src2.type == tt::ValueType::V_VEC4)
	{
		sm::vec4 src1_v4 = *evm::VMHelper::GetHandleValue<sm::vec4>(src1);
		sm::vec4 src2_v4 = *evm::VMHelper::GetHandleValue<sm::vec4>(src2);

		sm::vec4 ret = src1_v4 - src2_v4;

		evm::Value v;
		v.type = tt::ValueType::V_VEC4;
		v.as.handle = new evm::Handle<sm::vec4>(std::make_shared<sm::vec4>(ret));

		vm->SetRegister(r_dst, v);
	}
	else
	{
		throw std::runtime_error("Not Implemented!");
	}
}

void MathOpCodeImpl::Mul(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src1 = vm->NextByte();
	uint8_t r_src2 = vm->NextByte();

	if (r_src1 == 0xff || r_src2 == 0xff) 
	{
		evm::Value v;
		vm->SetRegister(r_dst, v);
		return;
	}

	auto& src1 = vm->GetRegister(r_src1);
	auto& src2 = vm->GetRegister(r_src2);
	if (src1.type == evm::ValueType::V_NUMBER &&
		src2.type == evm::ValueType::V_NUMBER)
	{
		evm::Value val;
		val.type = evm::ValueType::V_NUMBER;
		val.as.number = src1.as.number * src2.as.number;

		vm->SetRegister(r_dst, val);
	}
	else if (src1.type == tt::ValueType::V_VEC2 &&
		     src2.type == tt::ValueType::V_VEC2)
	{
		sm::vec2 src1_v2 = *evm::VMHelper::GetHandleValue<sm::vec2>(src1);
		sm::vec2 src2_v2 = *evm::VMHelper::GetHandleValue<sm::vec2>(src2);

		sm::vec2 ret = src1_v2 * src2_v2;

		evm::Value v;
		v.type = tt::ValueType::V_VEC2;
		v.as.handle = new evm::Handle<sm::vec2>(std::make_shared<sm::vec2>(ret));

		vm->SetRegister(r_dst, v);
	}
	else if (src1.type == tt::ValueType::V_VEC3 &&
		     src2.type == tt::ValueType::V_VEC3)
	{
		sm::vec3 src1_v3 = *evm::VMHelper::GetHandleValue<sm::vec3>(src1);
		sm::vec3 src2_v3 = *evm::VMHelper::GetHandleValue<sm::vec3>(src2);

		sm::vec3 ret = src1_v3 * src2_v3;

		evm::Value v;
		v.type = tt::ValueType::V_VEC3;
		v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

		vm->SetRegister(r_dst, v);
	}
	else if (src1.type == evm::ValueType::V_NUMBER &&
		     src2.type == tt::ValueType::V_VEC3)
	{
		sm::vec3 src2_v3 = *evm::VMHelper::GetHandleValue<sm::vec3>(src2);

		sm::vec3 ret = src2_v3 * static_cast<float>(src1.as.number);

		evm::Value v;
		v.type = tt::ValueType::V_VEC3;
		v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

		vm->SetRegister(r_dst, v);
	}
	else if (src2.type == evm::ValueType::V_NUMBER &&
		     src1.type == tt::ValueType::V_VEC3)
	{
		sm::vec3 src1_v3 = *evm::VMHelper::GetHandleValue<sm::vec3>(src1);

		sm::vec3 ret = src1_v3 * static_cast<float>(src2.as.number);

		evm::Value v;
		v.type = tt::ValueType::V_VEC3;
		v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

		vm->SetRegister(r_dst, v);
	}
	else
	{
		throw std::runtime_error("Not Implemented!");
	}
}

void MathOpCodeImpl::Div(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src1 = vm->NextByte();
	uint8_t r_src2 = vm->NextByte();

	if (r_src1 == 0xff || r_src2 == 0xff)
	{
		evm::Value v;
		vm->SetRegister(r_dst, v);
		return;
	}

	auto& src1 = vm->GetRegister(r_src1);
	auto& src2 = vm->GetRegister(r_src2);
	if (src1.type == evm::ValueType::V_NUMBER &&
		src2.type == evm::ValueType::V_NUMBER)
	{
		if (src2.as.number == 0) {
			throw std::runtime_error("Divide zero!");
		}

		evm::Value val;
		val.type = evm::ValueType::V_NUMBER;
		val.as.number = src1.as.number / src2.as.number;

		vm->SetRegister(r_dst, val);
	}
	else if (src1.type == tt::ValueType::V_VEC2 &&
		     src2.type == tt::ValueType::V_VEC2)
	{
		sm::vec2 src1_v2 = *evm::VMHelper::GetHandleValue<sm::vec2>(src1);
		sm::vec2 src2_v2 = *evm::VMHelper::GetHandleValue<sm::vec2>(src2);

		if (src2_v2.x == 0 || src2_v2.y == 0) {
			throw std::runtime_error("Divide zero!");
		}

		sm::vec2 ret = src1_v2 / src2_v2;

		evm::Value v;
		v.type = tt::ValueType::V_VEC2;
		v.as.handle = new evm::Handle<sm::vec2>(std::make_shared<sm::vec2>(ret));

		vm->SetRegister(r_dst, v);
	}
	else if (src1.type == evm::ValueType::V_NUMBER &&
		     src2.type == tt::ValueType::V_VEC3)
	{
		if (src1.as.number == 0) {
			throw std::runtime_error("Divide zero!");
		}

		sm::vec3 src2_v3 = *evm::VMHelper::GetHandleValue<sm::vec3>(src2);

		sm::vec3 ret = src2_v3 / static_cast<float>(src1.as.number);

		evm::Value v;
		v.type = tt::ValueType::V_VEC3;
		v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

		vm->SetRegister(r_dst, v);
	}
	else if (src2.type == evm::ValueType::V_NUMBER &&
		     src1.type == tt::ValueType::V_VEC3)
	{
		if (src2.as.number == 0) {
			throw std::runtime_error("Divide zero!");
		}

		sm::vec3 src1_v3 = *evm::VMHelper::GetHandleValue<sm::vec3>(src1);

		sm::vec3 ret = src1_v3 / static_cast<float>(src2.as.number);

		evm::Value v;
		v.type = tt::ValueType::V_VEC3;
		v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

		vm->SetRegister(r_dst, v);
	}
	else
	{
		throw std::runtime_error("Not Implemented!");
	}
}

}