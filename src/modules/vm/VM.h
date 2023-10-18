#pragma once

#include "core/macro.h"

#include <memory>

namespace evm { class VM; }

namespace tt
{

class OpFieldMap;

class VM
{
public:
	void Init(const std::shared_ptr<evm::VM>& vm);

	auto GetOpFields() const { return m_op_fields; }

private:
	std::shared_ptr<OpFieldMap> m_op_fields;

	TT_SINGLETON_DECLARATION(VM)

}; // VM

}