#pragma once

#include <map>
#include <vector>
#include <memory>

namespace tt
{

struct CodeBlock
{
	size_t hash;

	int begin;
	int end;

	int reg;

	int times = 0;

	int group;

	bool dirty = false;
};

class Bytecodes;

class CodesOptimize
{
public:
	void AddBlock(size_t hash, int begin, int end, int reg);

	std::shared_ptr<Bytecodes> RmDupCodes(const std::shared_ptr<Bytecodes>& codes) const;

	// rewrite
	void WriteNumber(int pos, float num);
	void FlushCache();

private:
	std::vector<std::vector<CodeBlock>> PrepareBlocks() const;

	int Relocate(int pos) const;

private:
	std::map<size_t, std::vector<CodeBlock>> m_blocks;

	mutable std::shared_ptr<Bytecodes> m_old_codes;
	mutable std::vector<std::vector<CodeBlock>> m_removed_blocks;

	mutable std::shared_ptr<Bytecodes> m_new_codes;

}; // CodesOptimize

}