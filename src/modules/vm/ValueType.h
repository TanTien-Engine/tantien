#pragma once

#include <easyvm/Value.h>

namespace tt
{

enum ValueType
{
	V_ARRAY = evm::ValueType::V_HANDLE + 1,

	V_VEC3,
	V_MAT4,

	V_PLANE,
	V_POLY_FACE,
	V_POLY,
};

}