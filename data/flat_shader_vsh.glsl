#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aVtxColor;

out vec2 TexCoord;
out vec4 VtxColor;

uniform mat4 model;

void main()
{
    gl_Position = model * vec4(aPos, 1.0f);
    TexCoord = vec2(aTexCoord.x, aTexCoord.y);
    VtxColor = vec4(aVtxColor);
}
