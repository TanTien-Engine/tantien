#include "stl_opcodes.h"
#include "VMHelper.h"
#include "ValueType.h"

#include <easyvm/VM.h>
#include <easyvm/VMHelper.h>

#include <polymesh3/Polytope.h>

#include <vector>

namespace
{

void vector_concat(const evm::Value& dst, const evm::Value& src)
{
	auto v_dst = tt::VMHelper::GetValArray(dst);
	if (src.type == tt::ValueType::V_ARRAY)
	{
		auto v_src = tt::VMHelper::GetValArray(src);
		for (auto& v : *v_src) {
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
	vm->RegistOperator(OP_VECTOR_FETCH_R, VectorFetchR);
	vm->RegistOperator(OP_VECTOR_SIZE, VectorSize);
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
	if (r_dst == 0xff || r_src == 0xff) {
		return;
	}

	auto vector = tt::VMHelper::GetRegArray(vm, r_dst);
	vector->push_back(vm->GetRegister(r_src));
}

void StlOpCodeImpl::VectorConcat(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_src = vm->NextByte();
	if (r_dst == 0xff || r_src == 0xff) {
		return;
	}

	vector_concat(vm->GetRegister(r_dst), vm->GetRegister(r_src));
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

void StlOpCodeImpl::VectorFetchR(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_vec = vm->NextByte();
	uint8_t r_idx = vm->NextByte();

	auto vec = tt::VMHelper::GetRegArray(vm, r_vec);
	if (!vec) {
		return;
	}

	auto idx = static_cast<int>(evm::VMHelper::GetRegNumber(vm, r_idx));
	if (idx < 0 || idx >= vec->size()) {
		return;
	}

	vm->SetRegister(r_dst, (*vec)[idx]);
}

void StlOpCodeImpl::VectorSize(evm::VM* vm)
{
	uint8_t r_dst = vm->NextByte();
	uint8_t r_vec = vm->NextByte();

	auto vec = tt::VMHelper::GetRegArray(vm, r_vec);
	if (!vec) {
		return;
	}

	evm::Value val;
	val.type = evm::ValueType::V_NUMBER;
	val.as.number = static_cast<t_num>(vec->size());

	vm->SetRegister(r_dst, val);
}

}