#include "math_opcodes.h"
#include "ValueType.h"

#include <SM_Vector.h>
#include <SM_Plane.h>
#include <SM_Matrix.h>
#include <easyvm/VM.h>
#include <easyvm/VMHelper.h>
#include <easyvm/Value.h>

namespace tt
{

void MathOpCodeImpl::OpCodeInit(evm::VM* vm)
{
	vm->RegistOperator(OP_VEC3_CREATE, Vec3Create3);
	vm->RegistOperator(OP_VEC3_STORE, Vec3Create);
	vm->RegistOperator(OP_VEC3_PRINT, Vec3Print);
	vm->RegistOperator(OP_VEC3_ADD, Vec3Add);
	vm->RegistOperator(OP_VEC3_SUB, Vec3Sub);
	vm->RegistOperator(OP_VEC3_TRANSFORM, Vec3Transform);
	vm->RegistOperator(OP_VEC3_GET_X, Vec3GetX);
	vm->RegistOperator(OP_VEC3_GET_Y, Vec3GetY);
	vm->RegistOperator(OP_VEC3_GET_Z, Vec3GetZ);

	vm->RegistOperator(OP_MATRIX_CREATE, MatrixCreate);
	vm->RegistOperator(OP_MATRIX_ROTATE, MatrixRotate);
	vm->RegistOperator(OP_MATRIX_TRANSLATE, MatrixTranslate);

	vm->RegistOperator(OP_MUL_UNKNOWN, MulUnknown);
}

void MathOpCodeImpl::Vec3Create3(evm::VM* vm)
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

void MathOpCodeImpl::Vec3Create(evm::VM* vm)
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
		vm->SetRegister(r_dst, evm::Value());
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
		vm->SetRegister(r_dst, evm::Value());
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

void MathOpCodeImpl::MulUnknown(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src1 = vm->NextByte();
	uint8_t r_src2 = vm->NextByte();

	evm::Value src1, src2;
	if (!vm->GetRegister(r_src1, src1)) {
		return;
	}
	if (!vm->GetRegister(r_src2, src2)) {
		return;
	}

	if (src1.type == evm::ValueType::V_NUMBER &&
		src2.type == evm::ValueType::V_NUMBER)
	{
		evm::Value val;
		val.type = evm::ValueType::V_NUMBER;
		val.as.number = src1.as.number * src2.as.number;

		vm->SetRegister(r_dst, val);
	}
	else if (src1.type == evm::ValueType::V_NUMBER &&
		     src2.type == tt::ValueType::V_VEC3)
	{
		sm::vec3 src2_v3 = *(static_cast<evm::Handle<sm::vec3>*>(src2.as.handle)->obj);

		sm::vec3 ret = src2_v3 * src1.as.number;

		evm::Value v;
		v.type = tt::ValueType::V_VEC3;
		v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

		vm->SetRegister(r_dst, v);
	}
	else if (src2.type == evm::ValueType::V_NUMBER &&
		     src1.type == tt::ValueType::V_VEC3)
	{
		sm::vec3 src1_v3 = *(static_cast<evm::Handle<sm::vec3>*>(src1.as.handle)->obj);

		sm::vec3 ret = src1_v3 * src2.as.number;

		evm::Value v;
		v.type = tt::ValueType::V_VEC3;
		v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

		vm->SetRegister(r_dst, v);
	}
	else if (src1.type == tt::ValueType::V_VEC3 &&
		     src2.type == tt::ValueType::V_VEC3)
	{
		sm::vec3 src1_v3 = *(static_cast<evm::Handle<sm::vec3>*>(src1.as.handle)->obj);
		sm::vec3 src2_v3 = *(static_cast<evm::Handle<sm::vec3>*>(src2.as.handle)->obj);

		sm::vec3 ret = src1_v3 * src2_v3;

		evm::Value v;
		v.type = tt::ValueType::V_VEC3;
		v.as.handle = new evm::Handle<sm::vec3>(std::make_shared<sm::vec3>(ret));

		vm->SetRegister(r_dst, v);
	}
	else
	{
		int zz = 0;
	}
}

}