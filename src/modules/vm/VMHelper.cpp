#include "VMHelper.h"
#include "modules/script/Proxy.h"

#include <vessel.h>
#include <easyvm/VM.h>

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
		v.type = evm::ValueType::HANDLE;
		v.as.handle = new evm::Handle<pm3::Polytope>(polytopes[0]);
#ifdef _DEBUG
		v.handle_type = "poly";
#endif // _DEBUG

		vm->SetRegister(reg, v);
	}
	else
	{
		auto list = std::make_shared<std::vector<evm::Value>>();

		for (auto src : polytopes)
		{
			evm::Value v;
			v.type = evm::ValueType::HANDLE;
			v.as.handle = new evm::Handle<pm3::Polytope>(src);
#ifdef _DEBUG
			v.handle_type = "poly";
#endif // _DEBUG

			list->push_back(v);
		}

		evm::Value v;
		v.type = evm::ValueType::ARRAY;
		v.as.handle = new evm::Handle<std::vector<evm::Value>>(list);
#ifdef _DEBUG
		v.handle_type = "vector";
#endif // _DEBUG

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

    if (val.type == evm::ValueType::HANDLE)
    {
        dst.push_back(static_cast<evm::Handle<pm3::Polytope>*>(val.as.handle)->obj);
    }
    else if (val.type == evm::ValueType::ARRAY)
    {
        auto src = static_cast<evm::Handle<std::vector<evm::Value>>*>(val.as.handle)->obj;
        for (auto p : *src) {
            dst.push_back(static_cast<evm::Handle<pm3::Polytope>*>(p.as.handle)->obj);
        }
    }

    return dst;
}

}