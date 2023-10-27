#pragma once

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

}