#include "stl_opcodes.h"
#include "VMHelper.h"
#include "ValueType.h"

#include <easyvm/VM.h>
#include <easyvm/VMHelper.h>

#include <polymesh3/Polytope.h>

#include <vector>

namespace
{

void vector_concat(evm::Value& dst, const evm::Value& src)
{
	auto v_dst = tt::VMHelper::GetValArray(dst);
	if (src.type == tt::ValueType::V_ARRAY)
	{
		auto v_src = tt::VMHelper::GetValArray(src);
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
	vm->RegistOperator(OP_VECTOR_GET, VectorGet);
}

void StlOpCodeImpl::VectorCreate(evm::VM* vm)
{
	uint8_t reg = vm->NextByte();

	auto vector = std::make_shared<std::vector<evm::Value>>();

	evm::Value val;
	val.type = tt::ValueType::V_ARRAY;
	val.as.handle = new evm::Handle<std::vector<evm::Value>>(vector);

	vm->SetRegister(reg, val);
}

void StlOpCodeImpl::VectorAdd(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src = vm->NextByte();

	auto vector = tt::VMHelper::GetRegArray(vm, r_dst);

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
	if (vm->GetRegister(r_dst, dst) && vm->GetRegister(r_src, src)) 
	{
		vector_concat(dst, src);

		// move
		evm::Value val;
		vm->MoveRegister(r_src, val);
	}
}

void StlOpCodeImpl::VectorGet(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_vec = vm->NextByte();
	uint8_t idx = vm->NextByte();

	auto vec = tt::VMHelper::GetRegArray(vm, r_vec);
	if (!vec || idx < 0 || idx >= vec->size()) {
		return;
	}

	vm->SetRegister(r_dst, (*vec)[idx]);
}

}