#include "VM.h"
#include "OpFieldMap.h"
#include "ValueCache.h"
#include "ValueType.h"

#include "math_opcodes.h"
#include "stl_opcodes.h"
#include "geo_opcodes.h"

#include <SM_Matrix.h>
#include <easyvm/VM.h>
#include <polymesh3/Polytope.h>

#include <stdexcept>

namespace
{

void free_value(evm::Value& val)
{
	if (val.type >= evm::ValueType::V_HANDLE && !val.as.handle) {
		return;
	}

	switch (val.type)
	{
	case tt::ValueType::V_ARRAY:
		static_cast<evm::Handle<std::vector<evm::Value>>*>(val.as.handle)->~Handle();
		break;
	case tt::ValueType::V_VEC2:
		static_cast<evm::Handle<sm::vec2>*>(val.as.handle)->~Handle();
		break;
	case tt::ValueType::V_VEC3:
		static_cast<evm::Handle<sm::vec3>*>(val.as.handle)->~Handle();
		break;
	case tt::ValueType::V_VEC4:
		static_cast<evm::Handle<sm::vec4>*>(val.as.handle)->~Handle();
		break;
	case tt::ValueType::V_MAT4:
		static_cast<evm::Handle<sm::mat4>*>(val.as.handle)->~Handle();
		break;
	case tt::ValueType::V_PLANE:
		static_cast<evm::Handle<sm::Plane>*>(val.as.handle)->~Handle();
		break;
	case tt::ValueType::V_CUBE:
		static_cast<evm::Handle<sm::cube>*>(val.as.handle)->~Handle();
		break;
	case tt::ValueType::V_POLY_POINT:
		static_cast<evm::Handle<pm3::Polytope::Point>*>(val.as.handle)->~Handle();
		break;
	case tt::ValueType::V_POLY_FACE:
		static_cast<evm::Handle<pm3::Polytope::Face>*>(val.as.handle)->~Handle();
		break;
	case tt::ValueType::V_POLY:
		static_cast<evm::Handle<pm3::Polytope>*>(val.as.handle)->~Handle();
		break;
	default:
		throw std::runtime_error("Unknown type!");
	}
}

}

namespace tt
{

TT_SINGLETON_DEFINITION(VM)

VM::VM()
{
	evm::Value::SetFreeCb(free_value);

	m_op_fields = std::make_shared<OpFieldMap>();
	m_cache = std::make_shared<ValueCache>();
}

std::shared_ptr<evm::VM> 
VM::CreateVM(const std::vector<uint8_t>& codes) const
{
	auto vm = std::make_shared<evm::VM>((char*)codes.data(), codes.size());

	MathOpCodeImpl::OpCodeInit(vm.get());
	StlOpCodeImpl::OpCodeInit(vm.get());
	GeoOpCodeImpl::OpCodeInit(vm.get());

	return vm;
}

}