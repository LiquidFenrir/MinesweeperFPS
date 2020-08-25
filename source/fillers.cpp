#include "fillers.h"

void Fillers::fill_quad_generic(VertexPtr& verts, const size_t idx, const PDD3 pos, const PDD2 uv, const glm::vec4 color)
{
    // A B
    // C D
    const Vertex vA{
        pos.p,
        uv.p,
        color
    };
    const Vertex vB{
        pos.p + pos.dr,
        uv.p + uv.dr,
        color
    };
    const Vertex vC{
        pos.p - pos.dd,
        uv.p - uv.dd,
        color
    };
    const Vertex vD{
        pos.p + pos.dr - pos.dd,
        uv.p + uv.dr - uv.dd,
        color
    };

    verts[mkidx(0, false, idx)] = vA;
    verts[mkidx(1, false, idx)] = vB;
    verts[mkidx(2, false, idx)] = vD;

    verts[mkidx(0, true, idx)] = vC;
    verts[mkidx(1, true, idx)] = vA;
    verts[mkidx(2, true, idx)] = vD;
}

int Fillers::mkidx(int vert_idx, int sechalf, int quad_idx)
{
    static constexpr int half[2] = {0, 3};
    return vert_idx + half[sechalf] + (quad_idx * 6);
}
