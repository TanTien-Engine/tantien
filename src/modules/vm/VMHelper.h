#pragma once

#include <vector>
#include <memory>

namespace evm { class VM; }
namespace pm3 { class Polytope; }

namespace tt
{

class VMHelper
{
public:
	static void StorePolys(evm::VM* vm, int reg, 
		const std::vector<std::shared_ptr<pm3::Polytope>>& polytopes);

	static std::vector<std::shared_ptr<pm3::Polytope>> LoadPolys(evm::VM* vm, int reg);

}; // VMHelper

}