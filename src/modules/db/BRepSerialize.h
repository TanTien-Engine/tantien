#pragma once

#include <SM_Vector.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace pm3 { class Polytope; }

namespace tt
{

class BrepSerialize
{
public:
	static void BRepToByteArray(const pm3::Polytope& brep, uint8_t** data, uint32_t& length);
	// `size` is the byte length of `data`; every read is bounds-checked against it.
	// Returns nullptr on a null/short/corrupt buffer instead of reading out of bounds.
	static std::shared_ptr<pm3::Polytope> BRepFromByteArray(const uint8_t* data, size_t size);

	static void BRepToByteArray(const std::vector<sm::vec3>& points, 
		const std::vector<std::vector<size_t>>& faces, uint8_t** data, uint32_t& length);

}; // BrepSerialize

}