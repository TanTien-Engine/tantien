#include "VM.h"

#include "math_opcodes.h"
#include "stl_opcodes.h"
#include "geo_opcodes.h"

namespace tt
{

void InitVM(const std::shared_ptr<evm::VM>& vm)
{
	MathOpCodeImpl::OpCodeInit(vm.get());
	StlOpCodeImpl::OpCodeInit(vm.get());
	GeoOpCodeImpl::OpCodeInit(vm.get());
}

}