#version 450 core

layout(location = 0) in vec2 uv;

layout(set = 0, binding = 0) uniform sampler2D colorInput;

layout(location = 0) out vec4 color;

void main() {
	
	color = vec4(texture(colorInput, uv).rgb, 1.);
//	color = vec4(uv, 1., 1.);
}