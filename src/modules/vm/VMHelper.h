#pragma once

#include <vector>
#include <memory>

namespace evm { class VM; struct Value; }
namespace pm3 { class Polytope; }

namespace tt
{

class VMHelper
{
public:
	static void StorePolys(evm::VM* vm, int reg, 
		const std::vector<std::shared_ptr<pm3::Polytope>>& polytopes);

	static std::vector<std::shared_ptr<pm3::Polytope>> LoadPolys(evm::VM* vm, int reg);

	static std::shared_ptr<std::vector<evm::Value>> GetRegArray(evm::VM* vm, int reg);
	static std::shared_ptr<std::vector<evm::Value>> GetValArray(const evm::Value& val);

}; // VMHelper

}