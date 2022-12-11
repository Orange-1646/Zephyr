#version 450 core

layout(location = 0) in vec2 uv;

layout(set = 0, binding = 0) uniform sampler2D colorInput;

layout(location = 0) out vec4 color;

vec3 ACESTonemap(vec3 color)
{
	mat3 m1 = mat3(
		0.59719, 0.07600, 0.02840,
		0.35458, 0.90834, 0.13383,
		0.04823, 0.01566, 0.83777
	);
	mat3 m2 = mat3(
		1.60475, -0.10208, -0.00327,
		-0.53108, 1.10813, -0.07276,
		-0.07367, -0.00605, 1.07602
	);
	vec3 v = m1 * color;
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return clamp(m2 * (a / b), 0.0, 1.0);
}

vec3 GammaCorrection(vec3 color, float gamma) {
	return pow(color, vec3(1.0f / gamma));
}


void main() {
	
	vec3 c = texture(colorInput, uv).rgb;

	if(c.r > 1.) {
		color = vec4(1., 0., 0., 1.);
	} else {
	color = vec4(vec3(0.), 1.);
	}

	vec3 hdrColor = texture(colorInput, uv).rgb;

	color = vec4(GammaCorrection(ACESTonemap(hdrColor), 2.2), 1.);
//	color = vec4(0.);
//	color = vec4(texture(colorInput, uv).rgb, 1.);
//	color = vec4(uv, 1., 1.);
}