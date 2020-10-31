#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 VtxColor;

// texture samplers
uniform sampler2D texture1;
uniform vec4 constColor;

void main()
{
	vec4 texel = constColor * (texture(texture1, TexCoord) * VtxColor);
	if(texel.a < 0.5)
		discard;
	FragColor = texel;
}
