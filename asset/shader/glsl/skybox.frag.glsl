#version 450 core

layout(location = 0) in vec3 inDirection;

layout(location = 0) out vec4 color;

layout(set = 0, binding = 1) uniform samplerCube skybox;

void main() {
	color = texture(skybox, inDirection);
}