#pragma once

#include "OpFieldType.h"

#include <array>
#include <vector>

namespace tt
{

#define MAX_OP_NUM 255

typedef std::vector<OpFieldType> OpFields;

class OpFieldMap
{
public:
	void Add(int op_type, const OpFields& fields);

	const OpFields* Query(int op_type) const;

private:
	std::array<OpFields, MAX_OP_NUM> m_opcodes;

}; // OpFieldMap

}