#version 450 core

const float EPSILON = 0.00001;

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba16) uniform image2D result;
layout(set = 0, binding = 1) uniform sampler2D inputImage;

layout(push_constant) uniform PrefilterData {
	vec4 data; // x: threshhold y: knee(threshold * soft threshold) z: 2 * knee w: .25/ knee
} prefilterData;

vec3 Filter(vec3 color) {
	float brightness = max(max(color.r, color.g), color.b);
	float soft = brightness - prefilterData.data.x + prefilterData.data.y;
	soft = clamp(soft, 0., prefilterData.data.z);
	soft = soft * soft * (prefilterData.data.w + EPSILON);

	float contrib = max(soft, brightness - prefilterData.data.x) / max(brightness, EPSILON);
	return color * contrib;
}

void main() {
	vec2 rtSize = imageSize(result);
	
	vec2 uv = vec2(float(gl_GlobalInvocationID.x)/rtSize.x, float(gl_GlobalInvocationID.y)/rtSize.y);
	uv += .5/ rtSize;

	vec3 color = texture(inputImage, uv).rgb;

	color = Filter(color);

	imageStore(result, ivec2(gl_GlobalInvocationID.xy), vec4(color, 1.));
}