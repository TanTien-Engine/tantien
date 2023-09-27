#include "stl_opcodes.h"

#include <easyvm/VM.h>
#include <easyvm/VMHelper.h>

#include <polymesh3/Polytope.h>

#include <vector>

namespace tt
{

void StlOpCodeImpl::OpCodeInit(evm::VM* vm)
{
	vm->RegistOperator(OP_VECTOR_CREATE, VectorCreate);
	vm->RegistOperator(OP_VECTOR_ADD, VectorAdd);
}

void StlOpCodeImpl::VectorCreate(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();

	auto vector = std::make_shared<std::vector<evm::Value>>();

	evm::Value val;
	val.type = evm::ValueType::HANDLE;
	val.as.handle = new evm::Handle<std::vector<evm::Value>>(vector);

	vm->SetRegister(reg, val);
}

void StlOpCodeImpl::VectorAdd(evm::VM* vm)
{
	uint8_t dst_reg = vm->NextByte();
	uint8_t src_reg = vm->NextByte();

	auto vector = evm::VMHelper::GetHandler<std::vector<evm::Value>>(vm, dst_reg);

	evm::Value val;
	// todo: del it in vector's dtor
	vm->MoveRegister(src_reg, val);
	vector->push_back(val);
}

}