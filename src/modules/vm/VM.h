#pragma once

#include <memory>

namespace evm { class VM; }

namespace tt
{

void InitVM(const std::shared_ptr<evm::VM>& vm);

}