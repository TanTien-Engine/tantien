#pragma once

#include "ValueCache.h"
#include "core/macro.h"

#include <easyvm/Value.h>

#include <memory>
#include <vector>

namespace evm { class VM; }

namespace tt
{

class OpFieldMap;

class VM
{
public:
	void Init(const std::shared_ptr<evm::VM>& vm);

	auto GetOpFields() const { return m_op_fields; }

	auto& GetCache() const { return m_cache; }

private:
	std::shared_ptr<OpFieldMap> m_op_fields = nullptr;

	std::shared_ptr<ValueCache> m_cache = nullptr;

	TT_SINGLETON_DECLARATION(VM)

}; // VM

}