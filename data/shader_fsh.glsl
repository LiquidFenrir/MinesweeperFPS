#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 VtxColor;

// texture samplers
uniform sampler2D texture1;
uniform vec4 constColor;

void main()
{
	FragColor = constColor * (texture(texture1, TexCoord) * VtxColor);
}
