#pragma once

#include "CodeBlock.h"

#include <map>
#include <vector>
#include <memory>

namespace tt
{

class Bytecodes;

class Optimizer
{
public:
	Optimizer(const std::shared_ptr<Bytecodes>& old_codes);

	void Optimize();

	// rewrite
	void WriteNumber(int pos, float num);
	void FlushCache();

	auto GetNewCodes() const { return m_new_codes; }

private:
	std::vector<std::vector<CodeBlock>> CalcBlocks() const;
	void CacheBlocks() const;

	int Relocate(int pos) const;

private:
	std::shared_ptr<Bytecodes> m_old_codes;
	mutable std::vector<std::vector<CodeBlock>> m_cached_blocks;

	mutable std::shared_ptr<Bytecodes> m_new_codes = nullptr;

}; // Optimizer

}