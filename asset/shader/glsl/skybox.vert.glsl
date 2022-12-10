#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inTexCoord;

layout(location = 0)out vec3 outDirection;

layout(set = 0, binding = 0) readonly buffer GlobalRenderData {
	mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 vp;
	vec3 directionalLightDirection;
	float _padding1;
	vec3 directionalLightRadiance;
	float _padding2;
	vec3 eye;
	float _padding3;
	mat4 lightVPCascade[4];
	vec2 cascadeSplits[4];
	vec4 cascadeSphereInfo[4];
} globalRenderData;

void main() {
	vec4 position = globalRenderData.projectionMatrix * mat4(mat3(globalRenderData.viewMatrix)) * vec4(inPosition.x, inPosition.y, inPosition.z, 1.);

	// make sure skybox always have z set to 1 to avoid overdraw
	gl_Position = position.xyww;

	outDirection = normalize(vec3(inPosition.x, inPosition.y, inPosition.z));
}