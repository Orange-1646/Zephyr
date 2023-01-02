#version 450 core

layout(local_size_x = 16, local_size_y = 16) in;
layout(set = 0, binding = 0, rgba16) uniform writeonly image2D result;
layout(set = 0, binding = 1) uniform sampler2D tex0;
layout(set = 0, binding = 2) uniform sampler2D tex1;


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

// Convert rgb to luminance
// with rgb in linear space with sRGB primaries and D65 white point
float LinearRgbToLuminance(vec3 linearRgb) {
	return dot(linearRgb, vec3(0.2126729f,  0.7151522f, 0.0721750f));
}

// we do the final composit + tonemapping + luminance preparation for fxaa here
void main() {
	vec2 invocID = vec2(gl_GlobalInvocationID);
	vec2 imgSize = imageSize(result);

	vec2 texCoords = (invocID + .5) / imgSize;

	vec3 color = textureLod(tex0, texCoords, 0).rgb + textureLod(tex1, texCoords, 0).rgb;

	color = ACESTonemap(color);

	imageStore(result, ivec2(gl_GlobalInvocationID), vec4(color, LinearRgbToLuminance(color)));
}