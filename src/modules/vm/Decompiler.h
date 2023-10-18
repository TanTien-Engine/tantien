#pragma once

#include <memory>
#include <vector>
#include <string>

namespace tt
{

class Bytecodes;
class OpFieldMap;

class Decompiler
{
public:
	Decompiler(const std::shared_ptr<Bytecodes>& codes,
		const std::shared_ptr<OpFieldMap>& ops);

	void Print(int begin, int end);

	size_t Hash(int begin, int end);

private:
	std::shared_ptr<Bytecodes> m_codes;
	std::shared_ptr<OpFieldMap> m_ops;

	static std::vector<std::string> m_op_names;

}; // Decompiler

}