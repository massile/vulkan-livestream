#version 450

layout(location = 0) in vec3 colorFrag;
layout(location = 1) in vec2 uvFrag;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D tex;

void main()
{
	outColor = texture(tex, uvFrag);
}
