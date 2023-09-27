#include "math_opcodes.h"

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
	vm->RegistOperator(OP_VEC3_STORE, Vec3Create);
	vm->RegistOperator(OP_VEC3_PRINT, Vec3Print);

	vm->RegistOperator(OP_MATRIX_CREATE, MatrixCreate);
	vm->RegistOperator(OP_MATRIX_ROTATE, MatrixRotate);
	vm->RegistOperator(OP_MATRIX_TRANSLATE, MatrixTranslate);
}

void MathOpCodeImpl::Vec3Create(evm::VM* vm)
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

void MathOpCodeImpl::Vec3Print(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();
	auto vec = evm::VMHelper::GetRegHandler<sm::vec3>(vm, reg);
	if (vec) {
		printf("%f, %f, %f", vec->x, vec->y, vec->z);
	}
}

void MathOpCodeImpl::MatrixCreate(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();

	auto mat = std::make_shared<sm::mat4>();

	evm::Value v;
	v.type = evm::ValueType::HANDLE;
	v.as.handle = new evm::Handle<sm::mat4>(mat);

	vm->SetRegister(reg, v);
}

void MathOpCodeImpl::MatrixRotate(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();

	auto mat = evm::VMHelper::GetRegHandler<sm::mat4>(vm, reg);
	if (!mat) {
		return;
	}

	uint8_t reg_xyz = vm->NextByte();
	auto vec = evm::VMHelper::GetRegHandler<sm::vec3>(vm, reg_xyz);
	if (!vec) {
		return;
	}

	mat->Rotate(vec->x, vec->y, vec->z);
}

void MathOpCodeImpl::MatrixTranslate(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();

	auto mat = evm::VMHelper::GetRegHandler<sm::mat4>(vm, reg);
	if (!mat) {
		return;
	}

	uint8_t reg_xyz = vm->NextByte();
	auto vec = evm::VMHelper::GetRegHandler<sm::vec3>(vm, reg_xyz);
	if (!vec) {
		return;
	}

	mat->Translate(vec->x, vec->y, vec->z);
}

}