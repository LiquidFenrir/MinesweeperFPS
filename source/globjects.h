#pragma once

#include <glm/glm.hpp>
#include <cstdio>

#define ATTRS_PACKED

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec4 col;
};

#ifdef ATTRS_PACKED
using VertexRef = Vertex&;
#else
class VertexRef {
    friend class VertexPtr;

    glm::vec3* const pos;
    glm::vec2* const uv;
    glm::vec4* const col;

    VertexRef(glm::vec3* pos_, glm::vec2* uv_, glm::vec4* col_) : pos(pos_), uv(uv_), col(col_)
    {

    }

public:
    Vertex operator=(const Vertex& vtx)
    {
        *pos = vtx.pos;
        *uv = vtx.uv;
        *col = vtx.col;
        return vtx;
    }
};
#endif

class VertexPtr {
    #ifdef ATTRS_PACKED
    Vertex* ptr;
    #else
    unsigned char* pos;
    unsigned char* uv;
    unsigned char* col;
    #endif

    const unsigned VBO;
    const size_t count;

public:

    VertexPtr(const unsigned v, const size_t cnt);
    // make sure to tell OpenGL we're done with the pointer
    ~VertexPtr();

    VertexRef operator[](const size_t idx);
};

class Buffer {
    const size_t count;
    unsigned VAO, VBO;

    Buffer(const size_t cnt);

public:
    Buffer(Buffer&&);

    static Buffer Verts(const size_t cnt)
    {
        return Buffer(cnt);
    }
    static Buffer Tris(const size_t cnt)
    {
        return Verts(cnt * 3);
    }
    static Buffer Quads(const size_t cnt)
    {
        return Tris(cnt * 2);
    }

    ~Buffer();

    void bind();
    void draw();

    VertexPtr getAllVerts();

    #ifdef ATTRS_PACKED
    void writeSingleQuad(const size_t idx, const Vertex* verts);
    #endif

};

class Texture
{
    friend class Framebuffer;
    unsigned tex;

public:
    Texture(const int width, const int height, const unsigned char* ptr = nullptr);
    ~Texture();

    void bind();
};

class Framebuffer {
    Texture out;
    const int width, height;
    unsigned framebuffer;

public:
    Framebuffer(const int w, const int h);
    ~Framebuffer();

    void bind();

    void bindOutput()
    {
        out.bind();
    }
};

void glCheckError_(const char *file, int line);
#define glCheckError() glCheckError_(__FILE__, __LINE__)
