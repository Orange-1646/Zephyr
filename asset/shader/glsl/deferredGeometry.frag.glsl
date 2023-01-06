// reference: https://google.github.io/filament/Filament.md.html#materialsystem

#version 450 core

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 uv;
layout(location = 2) in mat3 tbn;
layout(location = 5) in vec4 viewPos;

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D emissionMap;

layout(location = 0) out vec4 outAlbedoMetalness;
layout(location = 1) out vec4 outNormalRoughness;
layout(location = 2) out vec4 outPositionOcclusion;
layout(location = 3) out vec4 outEmissionShadow;


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
	vec3 n = normalize( (1. - materialUniforms.UseNormalMap) * normal);
	n = normalize(n);
	
	outAlbedoMetalness = vec4(albedo, metalness);
	outNormalRoughness = vec4(n, roughness * roughness);
	outPositionOcclusion = vec4(viewPos.xyz/viewPos.w, 1.);


	outEmissionShadow = vec4(materialUniforms.Emission, materialUniforms.ReceiveShadow);
}