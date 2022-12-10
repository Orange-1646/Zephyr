#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inTexCoord;

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

layout(push_constant, std140) uniform WorldMatrix
{
	mat4 worldMatrix;
	int cascadeIndex;
} worldMatrix;

void main() {
	gl_Position = globalRenderData.lightVPCascade[worldMatrix.cascadeIndex] * worldMatrix.worldMatrix * vec4(inPosition, 1.);
}