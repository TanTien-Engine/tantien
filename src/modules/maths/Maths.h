#pragma once

#include <SM_Plane.h>
#include <SM_Matrix.h>

namespace tt
{

class Maths
{
public:
	static void TransformPlane(sm::Plane& p, const sm::mat4& mt);

}; // Maths

}