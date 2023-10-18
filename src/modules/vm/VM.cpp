#include "VM.h"
#include "OpFieldMap.h"

#include "math_opcodes.h"
#include "stl_opcodes.h"
#include "geo_opcodes.h"

namespace tt
{

TT_SINGLETON_DEFINITION(VM)

VM::VM()
{
	m_op_fields = std::make_shared<OpFieldMap>();
}

void VM::Init(const std::shared_ptr<evm::VM>& vm)
{
	MathOpCodeImpl::OpCodeInit(vm.get());
	StlOpCodeImpl::OpCodeInit(vm.get());
	GeoOpCodeImpl::OpCodeInit(vm.get());
}

}