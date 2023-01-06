// reference: https://google.github.io/filament/Filament.md.html#materialsystem

#version 450 core

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 uv;
layout(location = 2) in mat3 tbn;
layout(location = 5) in vec4 viewPos;

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
layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D emissionMap;

layout(location = 0) out vec4 outAlbedoMetalness;
layout(location = 1) out vec4 outNormalRoughness;
layout(location = 2) out vec4 outPositionOcclusion;
layout(location = 3) out vec4 outEmissionShadow;

layout (depth_unchanged) out float gl_FragDepth;


layout(push_constant, std140) uniform Material
{
	layout(offset = 64)
	vec3 AlbedoColor;
	float Metalness;
	vec3 Emission;
	float Roughness;
	float UseNormalMap;
	float ReceiveShadow;
} materialUniforms;

void main() {

//------------------------------------------------------------------------------------------------------
// Lighting

	vec3 albedo = materialUniforms.AlbedoColor * texture(albedoMap, uv).xyz;
	vec3 emission = texture(emissionMap, uv).xyz + materialUniforms.Emission;
	float metalness = materialUniforms.Metalness * texture(metallicRoughnessMap, uv).b;
	float roughness = materialUniforms.Roughness * texture(metallicRoughnessMap, uv).g;
	
	roughness = clamp(roughness, .005, 1.);
	vec3 n = normalize(tbn * texture(normalMap, uv).xyz * materialUniforms.UseNormalMap + (1. - materialUniforms.UseNormalMap) * normal);
	n = normalize(n);

	vec3 v = normalize(globalRenderData.eye - viewPos.xyz/viewPos.w);
	vec3 tangent = tbn[0];
	vec3 bitangent = tbn[1];
	
	float anisotropy = 1.f;

	vec3 anisotropicDirection = anisotropy >= 0. ? bitangent : tangent;

	vec3 anisotropicTangent = cross(anisotropicDirection, v);
	vec3 anisotropicNormal = cross(anisotropicTangent, bitangent);


	vec3 bentNormal = normalize(mix(n, anisotropicNormal, anisotropy));
	
	outAlbedoMetalness = vec4(albedo, metalness);
	outNormalRoughness = vec4(bentNormal, roughness * roughness);

	// this actually stores tangent space vector
//	outPositionOcclusion = vec4(viewPos.xyz/viewPos.w, 1.);
	outPositionOcclusion = vec4(tbn[1], 1.);


	outEmissionShadow = vec4(materialUniforms.Emission, materialUniforms.ReceiveShadow);
}