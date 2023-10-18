#include "OpFieldMap.h"

namespace tt
{

void OpFieldMap::Add(int op_type, const std::vector<OpFieldType>& fields)
{
	if (op_type < 0 || op_type >= MAX_OP_NUM ||
		!m_opcodes[op_type].empty())
	{
		return;
	}

	m_opcodes[op_type] = fields;
}

const OpFields* OpFieldMap::Query(int op_type) const
{
	if (op_type < 0 || op_type >= MAX_OP_NUM) {
		return nullptr;
	}

	return &m_opcodes[op_type];
}

}