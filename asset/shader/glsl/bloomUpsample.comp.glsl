#version 450 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba16) uniform image2D result;
layout(set = 0, binding = 1) uniform sampler2D tex1;
layout(set = 0, binding = 2) uniform sampler2D tex2;

// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare

vec3 UpsampleTent9(sampler2D tex, float mip, vec2 uv, vec2 texelSize) {
//	texelSize *= .5;
	vec3 A = textureLod(tex, uv, mip).rgb * 4.;

	vec3 B = textureLod(tex, uv + vec2(-1., 1.) * texelSize, mip).rgb;
	vec3 C = textureLod(tex, uv + vec2(0., 1.) * texelSize, mip).rgb * 2.;
	vec3 D = textureLod(tex, uv + vec2(1., 1.) * texelSize, mip).rgb;
	vec3 E = textureLod(tex, uv + vec2(1., 0.) * texelSize, mip).rgb * 2;
	vec3 F = textureLod(tex, uv + vec2(1., -1.) * texelSize, mip).rgb;
	vec3 G = textureLod(tex, uv + vec2(0., -1.) * texelSize, mip).rgb * 2;
	vec3 H = textureLod(tex, uv + vec2(-1., -1.) * texelSize, mip).rgb;
	vec3 I = textureLod(tex, uv + vec2(-1., 0.) * texelSize, mip).rgb * 2;

	return (A + B + C + D + E + F + G + H + I) / 16.;
}

void main() {
	vec2 invocID = vec2(gl_GlobalInvocationID);

	vec2 resultSize = imageSize(result);
//	
//	if(invocID.x > resultSize.x || invocID.y > resultSize.y) {
//        return;
//    }
	vec2 texCoords = invocID/imageSize(result).xy;
    texCoords += (1.0f / resultSize) * 0.5f;

	// the color we composite on
	vec3 color1 = textureLod(tex1, texCoords, 0).rgb;

	vec2 texSize = textureSize(tex2, 0);
	vec2 texelSize = 1./texSize;
	// the upsampled color
//	vec3 color2 = UpsampleTent9(tex2, 0, texCoords, texelSize);
	vec3 color2 = textureLod(tex2, texCoords, 0).rgb;
	
	imageStore(result, ivec2(gl_GlobalInvocationID.xy), vec4(color1 + color2, 1.));
//	imageStore(result, ivec2(gl_GlobalInvocationID.xy), vec4(resultSize, 0., 1.));
}