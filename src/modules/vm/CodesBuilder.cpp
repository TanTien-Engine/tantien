#include "CodesBuilder.h"
#include "Bytecodes.h"
#include "OpFieldMap.h"

#include <stdexcept>

namespace tt
{

CodesBuilder::CodesBuilder(const std::shared_ptr<Bytecodes>& codes,
	                       const std::shared_ptr<OpFieldMap>& ops)
	: m_codes(codes)
	, m_ops(ops)
{
}

CodesBuilder::~CodesBuilder()
{
	m_ops->Add(m_op_type, m_op_fields);
}

void CodesBuilder::WriteType(uint8_t op)
{
	m_codes->Write(reinterpret_cast<const char*>(&op), sizeof(uint8_t));

	if (!m_op_fields.empty()) {
		throw std::runtime_error("OpType isn't the first!");
	}

	m_op_type = op;
	m_op_fields.push_back(OpFieldType::OpType);
}

void CodesBuilder::WriteReg(uint8_t reg)
{
	m_codes->Write(reinterpret_cast<const char*>(&reg), sizeof(uint8_t));

	m_op_fields.push_back(OpFieldType::Reg);
}

void CodesBuilder::WriteDouble(double d)
{
	m_codes->Write(reinterpret_cast<const char*>(&d), sizeof(double));

	m_op_fields.push_back(OpFieldType::Double);
}

void CodesBuilder::WriteFloat(float f)
{
	m_codes->Write(reinterpret_cast<const char*>(&f), sizeof(float));

	m_op_fields.push_back(OpFieldType::Float);
}

void CodesBuilder::WriteInt(int i)
{
	m_codes->Write(reinterpret_cast<const char*>(&i), sizeof(int));

	m_op_fields.push_back(OpFieldType::Int);
}

void CodesBuilder::WriteBool(bool b)
{
	m_codes->Write(reinterpret_cast<const char*>(&b), sizeof(bool));

	m_op_fields.push_back(OpFieldType::Bool);
}

}