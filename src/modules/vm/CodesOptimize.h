#pragma once

#include "CodeBlock.h"

#include <map>
#include <vector>
#include <memory>

namespace tt
{

class Bytecodes;

class CodesOptimize
{
public:
	std::shared_ptr<Bytecodes> RmDupCodes(const std::shared_ptr<Bytecodes>& codes,
		const std::map<size_t, std::vector<CodeBlock>>& blocks) const;

	// rewrite
	void WriteNumber(int pos, float num);
	void FlushCache();

private:
	std::vector<std::vector<CodeBlock>> PrepareBlocks(const std::map<size_t, std::vector<CodeBlock>>& blocks) const;

	int Relocate(int pos) const;

private:
	mutable std::shared_ptr<Bytecodes> m_old_codes;
	mutable std::vector<std::vector<CodeBlock>> m_removed_blocks;

	mutable std::shared_ptr<Bytecodes> m_new_codes;

}; // CodesOptimize

}