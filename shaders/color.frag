#version 450

layout(location = 0) out vec4 outColor;
in vec3 colorFrag;

void main()
{
	outColor = vec4(colorFrag, 1.0);
}
