#include "Decompiler.h"
#include "Bytecodes.h"
#include "OpFieldMap.h"
#include "math_opcodes.h"
#include "stl_opcodes.h"
#include "geo_opcodes.h"

#include <easyvm/OpCodes.h>

#include <stdexcept>
#include <map>
#include <algorithm>
#include <assert.h>

namespace
{

template<typename T>
T ReadData(const std::vector<uint8_t>& codes, int ip)
{
	T ret = 0;

	const int sz = sizeof(T);
	if (ip >= 0 && ip + sz < codes.size()) {
		memcpy(&ret, &codes[ip], sz);
	}

	return ret;
}

template<typename T>
void WriteData(std::vector<uint8_t>& codes, int ip, const T& val)
{
	memcpy(&codes[ip], &val, sizeof(T));
}

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

bool is_jump_op(int opcode)
{
	return opcode == evm::OP_JUMP
		|| opcode == evm::OP_JUMP_IF
		|| opcode == evm::OP_JUMP_IF_NOT;
}

}

namespace tt
{

std::vector<std::string> Decompiler::m_op_names;

Decompiler::Decompiler(const std::shared_ptr<Bytecodes>& codes,
	                   const std::shared_ptr<OpFieldMap>& ops)
	: m_codes(codes)
	, m_ops(ops)
{
	if (m_op_names.empty())
	{
		m_op_names.resize(MAX_OP_NUM);

		m_op_names[evm::OP_EXIT]			= "exit";
		m_op_names[evm::OP_MOVE_VAL]        = "move";
		m_op_names[evm::OP_BOOL_STORE]		= "bool_store";
		m_op_names[evm::OP_BOOL_PRINT]		= "bool_print";
		m_op_names[evm::OP_AND]				= "and";
		m_op_names[evm::OP_OR]				= "or";
		m_op_names[evm::OP_NUMBER_STORE]	= "num_store";
		m_op_names[evm::OP_NUMBER_PRINT]	= "num_print";
		m_op_names[evm::OP_NUMBER_NEGATE]	= "num_negate";
		m_op_names[evm::OP_ADD]				= "num_add";
		m_op_names[evm::OP_SUB]				= "num_sub";
		m_op_names[evm::OP_MUL]				= "num_mul";
		m_op_names[evm::OP_DIV]				= "num_div";
		m_op_names[evm::OP_INC]				= "inc";
		m_op_names[evm::OP_DEC]				= "dec";
		m_op_names[evm::OP_EQUAL]			= "equal";
		m_op_names[evm::OP_LESS]			= "less";
		m_op_names[evm::OP_JUMP]            = "jump";
		m_op_names[evm::OP_JUMP_IF]         = "jump_true";
		m_op_names[evm::OP_JUMP_IF_NOT]		= "jump_false";
		m_op_names[evm::OP_STRING_STORE]	= "str_store";
		m_op_names[evm::OP_STRING_PRINT]	= "str_print";

		m_op_names[tt::OP_VEC2_CREATE_I]	= "vec2";
		m_op_names[tt::OP_VEC3_CREATE_R]	= "vec3_r";
		m_op_names[tt::OP_VEC3_CREATE_I]	= "vec3_i";
		m_op_names[tt::OP_VEC3_PRINT]		= "vec3_print";
		m_op_names[tt::OP_VEC3_ADD]			= "vec3_add";
		m_op_names[tt::OP_VEC3_SUB]			= "vec3_sub";
		m_op_names[tt::OP_VEC3_TRANSFORM]	= "vec3_trans";
		m_op_names[tt::OP_VEC4_CREATE_I]	= "vec4_i";
		m_op_names[tt::OP_MATRIX_CREATE]	= "mat4";
		m_op_names[tt::OP_MATRIX_ROTATE]	= "mat4_rot";
		m_op_names[tt::OP_MATRIX_TRANSLATE] = "mat4_trans";
		m_op_names[tt::OP_CREATE_PLANE]		= "plane";
		m_op_names[tt::OP_CREATE_PLANE_2]	= "plane_2";
		m_op_names[tt::OP_CREATE_CUBE]		= "cube";
		m_op_names[tt::OP_GET_X]			= "get_x";
		m_op_names[tt::OP_GET_Y]			= "get_y";
		m_op_names[tt::OP_GET_Z]			= "get_z";
		m_op_names[tt::OP_GET_W]			= "get_w";
		m_op_names[tt::OP_ADD]				= "add";
		m_op_names[tt::OP_SUB]				= "sub";
		m_op_names[tt::OP_MUL]				= "mul";
		m_op_names[tt::OP_DIV]				= "div";
		m_op_names[tt::OP_NEGATE]			= "neg";
		m_op_names[tt::OP_ABS]              = "abs";

		m_op_names[tt::OP_VECTOR_CREATE]	= "array";
		m_op_names[tt::OP_VECTOR_ADD]		= "array_add";
		m_op_names[tt::OP_VECTOR_CONCAT]	= "array_conn";
		m_op_names[tt::OP_VECTOR_GET]		= "array_get";
		m_op_names[tt::OP_VECTOR_FETCH_R]   = "array_get_r";
		m_op_names[tt::OP_VECTOR_SIZE]      = "array_size";

		m_op_names[tt::OP_CREATE_POLYFACE]		= "face";
		m_op_names[tt::OP_CREATE_POLYTOPE]		= "poly";
		m_op_names[tt::OP_CREATE_POLYFACE_2]	= "face2";
		m_op_names[tt::OP_CREATE_POLYTOPE_2]	= "poly2";
		m_op_names[tt::OP_POLYTOPE_SUBTRACT]	= "poly_sub";
		m_op_names[tt::OP_POLYTOPE_EXTRUDE]		= "poly_ext";
		m_op_names[tt::OP_POLYTOPE_CLIP]		= "poly_clip";
		m_op_names[tt::OP_POLYTOPE_SET_DIRTY]	= "poly_dirty";
		m_op_names[tt::OP_POLYPOINT_SELECT]		= "poly_sel";
		m_op_names[tt::OP_POLYFACE_SELECT]		= "face_sel";
		m_op_names[tt::OP_POLY_COPY_FROM_MEM]	= "poly_from_mem";

		m_op_names[tt::OP_TRANSFORM]		= "transform";
	}
}

void Decompiler::Print(int begin, int end)
{
	auto& codes = m_codes->GetCode();
	if (begin < 0 || begin >= codes.size() || begin >= end) {
		return;
	}

	int ip = begin;
	while (ip < codes.size() && ip < end)
	{
		auto fields = m_ops->Query(codes[ip]);
		if (!fields) {
			throw std::runtime_error("Decompile fail!");
		}

		printf("%d: ", ip);

		for (size_t i = 0, n = fields->size(); i < n; ++i)
		{
			switch ((*fields)[i])
			{
			case OpFieldType::OpType:
				printf("%s", m_op_names[codes[ip]].c_str());
				ip += sizeof(uint8_t);
				break;
			case OpFieldType::Reg:
				printf("#%d", codes[ip]);
				ip += sizeof(uint8_t);
				break;
			case OpFieldType::Double:
				printf("%f", ReadData<double>(codes, ip));
				ip += sizeof(double);
				break;
			case OpFieldType::Float:
				printf("%f", ReadData<float>(codes, ip));
				ip += sizeof(float);
				break;
			case OpFieldType::Int:
				printf("%d", ReadData<int>(codes, ip));
				ip += sizeof(int);
				break;
			case OpFieldType::Bool:
				printf("%d", ReadData<bool>(codes, ip));
				ip += sizeof(bool);
				break;
			default:
				throw std::runtime_error("Unknown type!");
			}

			if (i == 0) {
				printf(" ");
			} else if (i != n - 1) {
				printf(", ");
			}
		}
		printf("\n");
	}

	printf("\n");
}

size_t Decompiler::Hash(int begin, int end)
{
	size_t hash = 0;

	auto& codes = m_codes->GetCode();
	if (begin < 0 || begin >= codes.size() || begin >= end) {
		return hash;
	}

	std::map<int, int> reg_map;
	int curr_reg = 0;

	int ip = begin;
	while (ip < codes.size() && ip < end)
	{
		auto fields = m_ops->Query(codes[ip]);
		if (!fields) {
			throw std::runtime_error("Hash fail!");
		}

		for (size_t i = 0, n = fields->size(); i < n; ++i)
		{
			switch ((*fields)[i])
			{
			case OpFieldType::OpType:
			{
				int type = codes[ip];
				hash_combine(hash, type);
				ip += sizeof(uint8_t);

				if (is_jump_op(type))
				{
					assert((*fields)[i + 1] == OpFieldType::Int);
					int offset = ReadData<int>(codes, ip);
					if (offset >= begin && offset < end)
					{
						hash_combine(hash, offset - begin);
					}
					else
					{
						assert(0);
					}
					ip += sizeof(int);
					++i;
				}
			}
				break;
			case OpFieldType::Reg:
			{
				int old_reg = codes[ip];
				int new_reg = 0;
				auto itr = reg_map.find(old_reg);
				if (itr == reg_map.end()) {
					new_reg = curr_reg++;
					reg_map.insert({ old_reg, new_reg });
				} else {
					new_reg = itr->second;
				}
				hash_combine(hash, new_reg);
				ip += sizeof(uint8_t);
			}
				break;
			case OpFieldType::Double:
				hash_combine(hash, ReadData<double>(codes, ip));
				ip += sizeof(double);
				break;
			case OpFieldType::Float:
				hash_combine(hash, ReadData<float>(codes, ip));
				ip += sizeof(float);
				break;
			case OpFieldType::Int:
				hash_combine(hash, ReadData<int>(codes, ip));
				ip += sizeof(int);
				break;
			case OpFieldType::Bool:
				hash_combine(hash, ReadData<bool>(codes, ip));
				ip += sizeof(bool);
				break;
			default:
				throw std::runtime_error("Unknown type!");
			}
		}
	}

	return hash;
}

void Decompiler::JumpLabelEncode()
{
	std::map<int, int> pos2lbl;

	auto& codes = m_codes->GetCode();

	int ip = 0;
	while (ip < codes.size())
	{
		auto fields = m_ops->Query(codes[ip]);
		if (!fields) {
			throw std::runtime_error("Error opcode!");
		}

		for (size_t i = 0, n = fields->size(); i < n; ++i)
		{
			switch ((*fields)[i])
			{
			case OpFieldType::OpType:
			{
				int type = codes[ip];
				ip += sizeof(uint8_t);

				if (is_jump_op(type))
				{
					assert((*fields)[i + 1] == OpFieldType::Int);
					int pos = ReadData<int>(codes, ip);

					int lbl = static_cast<int>(pos2lbl.size());
					WriteData<int>(const_cast<std::vector<uint8_t>&>(codes), ip, lbl);
					pos2lbl.insert({ pos, lbl });

					ip += sizeof(int);
					++i;
				}
			}
				break;
			case OpFieldType::Reg:
				ip += sizeof(uint8_t);
				break;
			case OpFieldType::Double:
				ip += sizeof(double);
				break;
			case OpFieldType::Float:
				ip += sizeof(float);
				break;
			case OpFieldType::Int:
				ip += sizeof(int);
				break;
			case OpFieldType::Bool:
				ip += sizeof(bool);
				break;
			default:
				throw std::runtime_error("Unknown type!");
			}
		}
	}

	auto& old_codes = m_codes->GetCode();
	std::vector<uint8_t> new_codes;

	int begin = 0;
	for (auto itr : pos2lbl)
	{
		int end = itr.first;
		std::copy(old_codes.begin() + begin, old_codes.begin() + end, std::back_inserter(new_codes));

		new_codes.push_back(0xff);
		new_codes.push_back(itr.second);

		begin = end;
	}
	std::copy(old_codes.begin() + begin, old_codes.end(), std::back_inserter(new_codes));

	m_codes->SetCode(new_codes);
}

void Decompiler::JumpLabelDecode()
{
	std::map<int, int> lbl2pos;
	std::map<int, int> pos2order;

	auto& codes = m_codes->GetCode();

	int ip = 0;
	while (ip < codes.size())
	{
		if (codes[ip] == 0xff)
		{
			int pos = ip;
			ip += sizeof(uint8_t);

			int lbl = codes[ip];
			ip += sizeof(uint8_t);

			lbl2pos.insert({ lbl, pos });

			int order = static_cast<int>(pos2order.size());
			pos2order.insert({ pos, order });

			continue;
		}

		auto fields = m_ops->Query(codes[ip]);
		if (!fields) {
			throw std::runtime_error("Error opcode!");
		}

		for (size_t i = 0, n = fields->size(); i < n; ++i)
		{
			switch ((*fields)[i])
			{
			case OpFieldType::OpType:
				ip += sizeof(uint8_t);
				break;
			case OpFieldType::Reg:
				ip += sizeof(uint8_t);
				break;
			case OpFieldType::Double:
				ip += sizeof(double);
				break;
			case OpFieldType::Float:
				ip += sizeof(float);
				break;
			case OpFieldType::Int:
				ip += sizeof(int);
				break;
			case OpFieldType::Bool:
				ip += sizeof(bool);
				break;
			default:
				throw std::runtime_error("Unknown type!");
			}
		}
	}

	for (auto& itr : lbl2pos)
	{
		int order = pos2order[itr.second];
		itr.second -= sizeof(uint8_t) * 2 * order;
	}

	ip = 0;
	while (ip < codes.size())
	{
		if (codes[ip] == 0xff)
		{
			ip += sizeof(uint8_t) * 2;
			continue;
		}

		auto fields = m_ops->Query(codes[ip]);
		if (!fields) {
			throw std::runtime_error("Error opcode!");
		}

		for (size_t i = 0, n = fields->size(); i < n; ++i)
		{
			switch ((*fields)[i])
			{
			case OpFieldType::OpType:
			{
				int type = codes[ip];
				ip += sizeof(uint8_t);

				if (is_jump_op(type))
				{
					assert((*fields)[i + 1] == OpFieldType::Int);
					int lbl = ReadData<int>(codes, ip);
					
					auto itr = lbl2pos.find(lbl);
					assert(itr != lbl2pos.end());
					int pos = itr->second;
					WriteData<int>(const_cast<std::vector<uint8_t>&>(codes), ip, pos);

					ip += sizeof(int);
					++i;
				}
			}
				break;
			case OpFieldType::Reg:
				ip += sizeof(uint8_t);
				break;
			case OpFieldType::Double:
				ip += sizeof(double);
				break;
			case OpFieldType::Float:
				ip += sizeof(float);
				break;
			case OpFieldType::Int:
				ip += sizeof(int);
				break;
			case OpFieldType::Bool:
				ip += sizeof(bool);
				break;
			default:
				throw std::runtime_error("Unknown type!");
			}
		}
	}

	auto& old_codes = m_codes->GetCode();
	std::vector<uint8_t> new_codes;

	int begin = 0;
	for (auto itr : pos2order)
	{
		int end = itr.first;
		std::copy(old_codes.begin() + begin, old_codes.begin() + end, std::back_inserter(new_codes));

		begin = end + 2;
	}
	std::copy(old_codes.begin() + begin, old_codes.end(), std::back_inserter(new_codes));

	m_codes->SetCode(new_codes);
}

void Decompiler::JumpLabelRelocate(const std::vector<CodeBlock>& rm_blocks)
{
	auto& codes = m_codes->GetCode();

	int ip = 0;
	while (ip < codes.size())
	{
		auto fields = m_ops->Query(codes[ip]);
		if (!fields) {
			throw std::runtime_error("Error opcode!");
		}

		for (size_t i = 0, n = fields->size(); i < n; ++i)
		{
			switch ((*fields)[i])
			{
			case OpFieldType::OpType:
			{
				int type = codes[ip];
				ip += sizeof(uint8_t);

				if (is_jump_op(type))
				{
					assert((*fields)[i + 1] == OpFieldType::Int);
					int old_pos = ReadData<int>(codes, ip);

					int new_pos = old_pos;
					for (auto& b : rm_blocks)
					{
						if (old_pos <= b.begin)
						{
							break;
						}
						else if (old_pos > b.begin && old_pos < b.end)
						{
							assert(ip >= b.begin && ip < b.end);
						}
						else
						{
							assert(old_pos >= b.end);
							// rm block
							new_pos -= (b.end - b.begin);
							// add OP_POLY_COPY_FROM_MEM
							new_pos += sizeof(uint8_t) * 3;
						}
					}

					if (new_pos != old_pos) {
						WriteData<int>(const_cast<std::vector<uint8_t>&>(codes), ip, new_pos);
					}

					ip += sizeof(int);
					++i;
				}
			}
				break;
			case OpFieldType::Reg:
				ip += sizeof(uint8_t);
				break;
			case OpFieldType::Double:
				ip += sizeof(double);
				break;
			case OpFieldType::Float:
				ip += sizeof(float);
				break;
			case OpFieldType::Int:
				ip += sizeof(int);
				break;
			case OpFieldType::Bool:
				ip += sizeof(bool);
				break;
			default:
				throw std::runtime_error("Unknown type!");
			}
		}
	}
}

}