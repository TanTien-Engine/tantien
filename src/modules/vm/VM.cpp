#include "VM.h"
#include "stl_op_codes.h"
#include "geo_op_codes.h"

namespace tt
{

void InitVM(const std::shared_ptr<evm::VM>& vm)
{
	StlOpCodeImpl::OpCodeInit(vm.get());
	GeoOpCodeImpl::OpCodeInit(vm.get());
}

}