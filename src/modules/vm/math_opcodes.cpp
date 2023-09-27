#include "math_opcodes.h"

#include <SM_Vector.h>
#include <SM_Plane.h>
#include <easyvm/VM.h>
#include <easyvm/VMHelper.h>
#include <easyvm/Value.h>

namespace tt
{

void MathOpCodeImpl::OpCodeInit(evm::VM* vm)
{
	vm->RegistOperator(OP_VEC3_STORE, Vec3Create);
	vm->RegistOperator(OP_VEC3_PRINT, Vec3Print);
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
	auto vec = evm::VMHelper::GetHandler<sm::vec3>(vm, reg);
	if (vec) {
		printf("%f, %f, %f", vec->x, vec->y, vec->z);
	}
}

}