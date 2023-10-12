#include "modules/maths/Maths.h"

namespace tt
{

void Maths::TransformPlane(sm::Plane& p, const sm::mat4& mt)
{
    auto& norm = p.normal;

    sm::vec3 pos0;
    if (norm.x != 0)
    {
        pos0.x = -p.dist / norm.x;
        pos0.y = 0;
        pos0.z = 0;
    }
    else if (norm.y != 0)
    {
        pos0.x = 0;
        pos0.y = -p.dist / norm.y;
        pos0.z = 0;
    }
    else if (norm.z != 0)
    {
        pos0.x = 0;
        pos0.y = 0;
        pos0.z = -p.dist / norm.z;
    }

    auto pos1 = pos0 + norm;

    pos0 = mt * pos0;
    pos1 = mt * pos1;
    p.Build(pos1 - pos0, pos0);
}

}