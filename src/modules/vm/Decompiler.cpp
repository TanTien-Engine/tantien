#include "Decompiler.h"
#include "Bytecodes.h"
#include "OpFieldMap.h"
#include "math_opcodes.h"
#include "stl_opcodes.h"
#include "geo_opcodes.h"

#include <easyvm/OpCodes.h>

#include <stdexcept>
#include <map>
#include <assert.h>

namespace
{

template<typename T>
static T ReadData(const std::vector<uint8_t>& codes, int ip)
{
	T ret = 0;

	const int sz = sizeof(T);
	if (ip >= 0 && ip + sz < codes.size()) {
		memcpy(&ret, &codes[ip], sz);
	}

	return ret;
}

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
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
		m_op_names[evm::OP_BOOL_STORE]		= "bool_store";
		m_op_names[evm::OP_BOOL_PRINT]		= "bool_print";
		m_op_names[evm::OP_NUMBER_STORE]	= "num_store";
		m_op_names[evm::OP_NUMBER_PRINT]	= "num_print";
		m_op_names[evm::OP_NUMBER_NEGATE]	= "num_negate";
		m_op_names[evm::OP_ADD]				= "num_add";
		m_op_names[evm::OP_SUB]				= "num_sub";
		m_op_names[evm::OP_MUL]				= "num_mul";
		m_op_names[evm::OP_DIV]				= "num_div";
		m_op_names[evm::OP_INC]				= "inc";
		m_op_names[evm::OP_DEC]				= "dec";
		m_op_names[evm::OP_CMP]				= "cmp";
		m_op_names[evm::OP_JUMP_IF_NOT]		= "jump_nz";
		m_op_names[evm::OP_STRING_STORE]	= "str_store";
		m_op_names[evm::OP_STRING_PRINT]	= "str_print";

		m_op_names[tt::OP_VEC2_CREATE_I]	= "vec2";
		m_op_names[tt::OP_VEC3_CREATE_R]	= "vec3_r";
		m_op_names[tt::OP_VEC3_CREATE_I]	= "vec3_i";
		m_op_names[tt::OP_VEC3_PRINT]		= "vec3_print";
		m_op_names[tt::OP_VEC3_ADD]			= "vec3_add";
		m_op_names[tt::OP_VEC3_SUB]			= "vec3_sub";
		m_op_names[tt::OP_VEC3_TRANSFORM]	= "vec3_trans";
		m_op_names[tt::OP_VEC3_GET_X]		= "vec3_x";
		m_op_names[tt::OP_VEC3_GET_Y]		= "vec3_y";
		m_op_names[tt::OP_VEC3_GET_Z]		= "vec3_z";
		m_op_names[tt::OP_VEC4_CREATE_I]	= "vec4_i";
		m_op_names[tt::OP_MATRIX_CREATE]	= "mat4";
		m_op_names[tt::OP_MATRIX_ROTATE]	= "mat4_rot";
		m_op_names[tt::OP_MATRIX_TRANSLATE] = "mat4_trans";
		m_op_names[tt::OP_CREATE_PLANE]		= "plane";
		m_op_names[tt::OP_CREATE_PLANE_2]	= "plane_2";
		m_op_names[tt::OP_CREATE_CUBE]		= "cube";
		m_op_names[tt::OP_ADD]				= "add";
		m_op_names[tt::OP_SUB]				= "sub";
		m_op_names[tt::OP_MUL]				= "mul";
		m_op_names[tt::OP_DIV]				= "div";

		m_op_names[tt::OP_VECTOR_CREATE]	= "array";
		m_op_names[tt::OP_VECTOR_ADD]		= "array_add";
		m_op_names[tt::OP_VECTOR_CONCAT]	= "array_conn";
		m_op_names[tt::OP_VECTOR_GET]		= "array_get";

		m_op_names[tt::OP_CREATE_POLYFACE]		= "face";
		m_op_names[tt::OP_CREATE_POLYTOPE]		= "poly";
		m_op_names[tt::OP_CREATE_POLYFACE_2]	= "face2";
		m_op_names[tt::OP_CREATE_POLYTOPE_2]	= "poly2";
		m_op_names[tt::OP_POLYTOPE_TRANSFORM]	= "poly_trans";
		m_op_names[tt::OP_POLYTOPE_SUBTRACT]	= "poly_sub";
		m_op_names[tt::OP_POLYTOPE_EXTRUDE]		= "poly_ext";
		m_op_names[tt::OP_POLYTOPE_CLIP]		= "poly_clip";
		m_op_names[tt::OP_POLYTOPE_SET_DIRTY]	= "poly_dirty";
		m_op_names[tt::OP_POLYPOINT_SELECT]		= "poly_sel";
		m_op_names[tt::OP_POLYFACE_SELECT]		= "face_sel";
		m_op_names[tt::OP_TRANSFORM_UNKNOWN]	= "transform";
		m_op_names[tt::OP_POLY_COPY_FROM_MEM]	= "poly_from_mem";
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

				if (type == evm::OP_JUMP_IF_NOT)
				{
					assert((*fields)[i + 1] == OpFieldType::Int);
					int offset = ReadData<int>(codes, ip);
					if (offset >= begin && offset < end)
					{
						hash_combine(hash, offset - begin);
						ip += sizeof(int);
						++i;
					}
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

}