#pragma once

#include <glm/glm.hpp>
#include "globjects.h"

// TopLeft, delta right, delta down
template<typename T>
struct PositionDeltaDelta {
    T p, dr, dd;
};

using PDD3 = PositionDeltaDelta<glm::vec3>;
using PDD2 = PositionDeltaDelta<glm::vec2>;

namespace Fillers {
    void fill_quad_generic(VertexPtr& verts, const size_t idx, const PDD3 pos, const PDD2 uv, const glm::vec4 color);
    int mkidx(int vert_idx, int sechalf, int quad_idx);
}
