#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inTexCoord;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out mat3 tbn;
layout(location = 5) out vec4 viewPos;

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
} worldMatrix;

void main() {

	gl_Position = globalRenderData.vp * worldMatrix.worldMatrix * vec4(inPosition, 1.);
	viewPos = worldMatrix.worldMatrix * vec4(inPosition, 1.);

	// Gram-Schmidt Orthogonalization
	vec3 t = inTangent;
	vec3 n = inNormal;

	t = normalize(t - dot(t, n)*n);
	vec3 b = cross(n, t);
	tbn = mat3(worldMatrix.worldMatrix) * mat3(t, b, n);

	outNormal =  mat3(worldMatrix.worldMatrix) * inNormal;
	outTexCoord = vec2(inTexCoord.x, inTexCoord.y);
}