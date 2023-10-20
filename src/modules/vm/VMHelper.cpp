#include "VMHelper.h"
#include "ValueType.h"
#include "modules/script/Proxy.h"

#include <vessel.h>
#include <easyvm/VM.h>

#include <assert.h>

namespace
{

void load_polys(std::vector<std::shared_ptr<pm3::Polytope>>& dst, const evm::Value& src)
{
	switch (src.type)
	{
	case tt::ValueType::V_POLY:
	{
		auto poly = static_cast<evm::Handle<pm3::Polytope>*>(src.as.handle)->obj;
		dst.push_back(poly);
	}
		break;
	case  tt::ValueType::V_ARRAY:
	{
		auto items = static_cast<evm::Handle<std::vector<evm::Value>>*>(src.as.handle)->obj;
		for (auto item : *items) {
			load_polys(dst, item);
		}
	}
		break;
	default:
		assert(0);
	}
}

}

namespace tt
{

void VMHelper::StorePolys(evm::VM* vm, int reg,
	                      const std::vector<std::shared_ptr<pm3::Polytope>>& polytopes)
{
	if (polytopes.empty()) 
	{
		vm->SetRegister(reg, evm::Value());
	}
	else if (polytopes.size() == 1)
	{
		evm::Value v;
		v.type = tt::ValueType::V_POLY;
		v.as.handle = new evm::Handle<pm3::Polytope>(polytopes[0]);

		vm->SetRegister(reg, v);
	}
	else
	{
		auto list = std::make_shared<std::vector<evm::Value>>();

		for (auto src : polytopes)
		{
			evm::Value v;
			v.type = tt::ValueType::V_POLY;
			v.as.handle = new evm::Handle<pm3::Polytope>(src);

			list->push_back(v);
		}

		evm::Value v;
		v.type = tt::ValueType::V_ARRAY;
		v.as.handle = new evm::Handle<std::vector<evm::Value>>(list);

		vm->SetRegister(reg, v);
	}
}

std::vector<std::shared_ptr<pm3::Polytope>> 
VMHelper::LoadPolys(evm::VM* vm, int reg)
{
    std::vector<std::shared_ptr<pm3::Polytope>> dst;

    evm::Value val;
    if (!vm->GetRegister(reg, val)) {
        return dst;
    }

	load_polys(dst, val);

    return dst;
}

std::shared_ptr<std::vector<evm::Value>> VMHelper::GetRegArray(evm::VM* vm, int reg)
{
	evm::Value val;
	if (!vm->GetRegister(reg, val)) {
		vm->Error("Error reg.");
		return nullptr;
	}

	if (val.type != tt::ValueType::V_ARRAY) {
		vm->Error("The register doesn't contain a array.");
		return nullptr;
	}

	return GetValArray(val);
}

std::shared_ptr<std::vector<evm::Value>> VMHelper::GetValArray(const evm::Value& val)
{
	if (val.type == tt::ValueType::V_ARRAY) {
		return static_cast<evm::Handle<std::vector<evm::Value>>*>(val.as.handle)->obj;
	} else {
		return nullptr;
	}
}

}