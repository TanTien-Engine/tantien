#include "stl_opcodes.h"

#include <easyvm/VM.h>
#include <easyvm/VMHelper.h>

#include <polymesh3/Polytope.h>

#include <vector>

namespace
{

void vector_concat(evm::Value& dst, const evm::Value& src)
{
	auto v_dst = evm::VMHelper::GetValArray(dst);
	if (src.type == evm::ValueType::ARRAY)
	{
		auto v_src = evm::VMHelper::GetValArray(src);
		for (auto v : *v_src) {
			vector_concat(dst, v);
		}
	}
	else
	{
		v_dst->push_back(src);
	}
};

}

namespace tt
{

void StlOpCodeImpl::OpCodeInit(evm::VM* vm)
{
	vm->RegistOperator(OP_VECTOR_CREATE, VectorCreate);
	vm->RegistOperator(OP_VECTOR_ADD, VectorAdd);
	vm->RegistOperator(OP_VECTOR_CONCAT, VectorConcat);
}

void StlOpCodeImpl::VectorCreate(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();

	auto vector = std::make_shared<std::vector<evm::Value>>();

	evm::Value val;
	val.type = evm::ValueType::ARRAY;
	val.as.handle = new evm::Handle<std::vector<evm::Value>>(vector);
#ifdef _DEBUG
	val.handle_type = "vector";
#endif // _DEBUG

	vm->SetRegister(reg, val);
}

void StlOpCodeImpl::VectorAdd(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src = vm->NextByte();

	auto vector = evm::VMHelper::GetRegArray(vm, r_dst);

	evm::Value val;
	// todo: del it in vector's dtor
	vm->MoveRegister(r_src, val);
	vector->push_back(val);
}

void StlOpCodeImpl::VectorConcat(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src = vm->NextByte();

	evm::Value dst, src;
	if (vm->GetRegister(r_dst, dst) && vm->GetRegister(r_src, src)) {
		vector_concat(dst, src);
	}
}

}