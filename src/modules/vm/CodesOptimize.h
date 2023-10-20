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
};

class Bytecodes;

class CodesOptimize
{
public:
	void AddBlock(size_t hash, int begin, int end, int reg);

	void RmDupCodes(const std::shared_ptr<Bytecodes>& codes);

private:
	std::vector<std::vector<CodeBlock>> PrepareBlocks() const;

private:
	std::map<size_t, std::vector<CodeBlock>> m_blocks;

}; // CodesOptimize

}