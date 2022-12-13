#version 450 core

layout(local_size_x = 16, local_size_y = 16) in;
layout(set = 0, binding = 0, rgba16) uniform writeonly image2D result;
layout(set = 0, binding = 1) uniform sampler2D tex0;
layout(set = 0, binding = 2) uniform sampler2D tex1;

void main() {
	vec2 invocID = vec2(gl_GlobalInvocationID);
	vec2 imgSize = imageSize(result);

	vec2 texCoords = (invocID + .5) / imgSize;

	vec3 color = textureLod(tex0, texCoords, 0).rgb + textureLod(tex1, texCoords, 0).rgb;

	imageStore(result, ivec2(gl_GlobalInvocationID), vec4(color, 1.));
}