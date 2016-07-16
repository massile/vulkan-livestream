#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform UBO
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
} ubo;

out vec3 colorFrag;

void main()
{
	colorFrag = color;
	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(position, 1.0);
}
