#include "globjects.h"

#include <tuple>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

VertexPtr::VertexPtr(const unsigned v, const size_t cnt) : VBO(v), count(cnt)
{
    #ifndef ATTRS_PACKED
    unsigned char* ptr = nullptr;
    #endif
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    ptr = static_cast<decltype(ptr)>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
    #ifndef ATTRS_PACKED
    pos = ptr + cnt * ((uintptr_t)offsetof(Vertex, pos);
    uv = ptr + cnt * ((uintptr_t)offsetof(Vertex, uv);
    col = ptr + cnt * ((uintptr_t)offsetof(Vertex, col);
    #endif
}
VertexPtr::~VertexPtr()
{
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glUnmapBuffer(GL_ARRAY_BUFFER);
}
VertexRef VertexPtr::operator[](const size_t idx)
{
    #ifdef ATTRS_PACKED
    return ptr[idx];
    #else
    #define DO_PART(p) static_cast<void*>(p + idx * sizeof(Vertex::p))
    return VertexRef(
        DO_PART(pos),
        DO_PART(uv),
        DO_PART(col)
    );
    #undef DO_PART
    #endif
}

Buffer::Buffer(const size_t cnt) : count(cnt)
{
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    const size_t total_size = sizeof(Vertex) * cnt;
    glBufferData(GL_ARRAY_BUFFER, total_size, nullptr, GL_DYNAMIC_DRAW);

    std::tuple<int, size_t, uintptr_t> attrinfo[] = {
    #ifdef ATTRS_PACKED
    {3, sizeof(Vertex), (uintptr_t)offsetof(Vertex, pos)},
    {2, sizeof(Vertex), (uintptr_t)offsetof(Vertex, uv)},
    {4, sizeof(Vertex), (uintptr_t)offsetof(Vertex, col)},
    #else
    {3, sizeof(Vertex::pos), cnt * (uintptr_t)offsetof(Vertex, pos)},
    {2, sizeof(Vertex::uv), cnt * (uintptr_t)offsetof(Vertex, uv)},
    {4, sizeof(Vertex::col), cnt * (uintptr_t)offsetof(Vertex, col)},
    #endif
    };

    int idx = 0;
    for(const auto [floatcnt, sz, off] : attrinfo)
    {
        glVertexAttribPointer(idx, floatcnt, GL_FLOAT, GL_FALSE, sz, (void*)off);
        glEnableVertexAttribArray(idx);
        idx++;
    }
}
Buffer::Buffer(Buffer&& b) : count(b.count), VAO(b.VAO), VBO(b.VBO)
{
    b.VAO = 0;
    b.VBO = 0;
}
Buffer::~Buffer()
{
    if(VAO) {
        glDeleteVertexArrays(1, &VAO);
    } 
    if(VBO) {
        glDeleteBuffers(1, &VBO);
    }
}
void Buffer::bind()
{
    glBindVertexArray(VAO);
}
void Buffer::draw()
{
    glDrawArrays(GL_TRIANGLES, 0, count);
}

VertexPtr Buffer::getAllVerts()
{
    return VertexPtr(VBO, count);
}
#ifdef ATTRS_PACKED
void Buffer::writeSingleQuad(const size_t idx, const Vertex* verts)
{
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, idx * sizeof(Vertex) * 6, sizeof(Vertex) * 6, verts);
}
#endif

Texture::Texture(const int width, const int height, const unsigned char* ptr)
{
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // load image, create texture and generate mipmaps
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
}
Texture::~Texture()
{
    glDeleteTextures(1, &tex);
}
void Texture::bind()
{
    glBindTexture(GL_TEXTURE_2D, tex);
}

Framebuffer::Framebuffer(const int w, const int h) : out(w, h), width(w), height(h)
{
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, out.tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &framebuffer);
}
void Framebuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);
}
