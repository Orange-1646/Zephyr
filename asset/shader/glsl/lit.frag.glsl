// reference: https://google.github.io/filament/Filament.md.html#materialsystem

#version 450 core

#define M_PI 3.1415926535897932384626433832795
const float Epsilon = 0.00001;
const float ShadowBias = 0.002;
#define MAX_POINT_LIGHT_COUNT 1000

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 uv;
layout(location = 2) in mat3 tbn;
layout(location = 5) in vec4 viewPos;

layout(location = 0) out vec4 color;

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

layout(set = 0, binding = 1) uniform samplerCube skybox;
layout(set = 0, binding = 2) uniform sampler2DArray shadowMap;
struct PointLight {
	vec3 position;
	float radius;
	vec3 radiance;
	float falloff;
};
layout(set = 0, binding = 3) uniform PointLightData {
	PointLight lights[MAX_POINT_LIGHT_COUNT];
	uint lightCount;
} u_PointLights;

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D emissionMap;

layout(push_constant, std140) uniform Material
{
	layout(offset = 64)
	vec3 AlbedoColor;
	float Metalness;
	vec3 Emission;
	float Roughness;
	float UseNormalMap;
} materialUniforms;

struct PBRParameters {
	vec3 albedo;
	vec3 position;
	float metalness;
	vec3 normal;
	float roughness;
	vec3 view;
	float NoV;
};

vec3 F_Schlick(vec3 f0, float VoH) {
	return f0 + (1. - f0) * pow(clamp(1. - VoH, 0., 1.), 5.);
}

float D_GGX(float roughness, float NoH) {
	float a = NoH * roughness;
	float k = roughness/(1. - NoH * NoH + roughness * roughness);
	return k * k / M_PI;
}

float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
	float a2 = roughness * roughness;
	float GGXV = NoL * sqrt(NoV * NoV * (1. - a2) + a2);
	float GGXL = NoV * sqrt(NoL * NoL * (1. - a2) + a2);
	
	return 0.5 / (GGXV + GGXL);
}

float Fd_Lambertian() {
	return 1. / M_PI;
}

const vec3 dielectricReflectance = vec3(0.04);

float unpack(vec4 rgbaDepth) {
    const vec4 bitShift = vec4(1.0, 1.0/256.0, 1.0/(256.0*256.0), 1.0/(256.0*256.0*256.0));
    return dot(rgbaDepth, bitShift);
}

// 7*7 box filter
float PCF_Box7x7(vec2 shadowUv, float objectDepth, float bias, uint cascade) {

	ivec2 texSize = textureSize(shadowMap, 0).xy;

	float percentage = 1.;

	for(float i = -3.; i < 3.; i++) {
		for(float j = -3.; j < 3.; j++) {
			vec2 uv = shadowUv + vec2(i / texSize.x, j / texSize.y);
			float sampledDepth = texture(shadowMap, vec3(uv, cascade)).r;

			if(sampledDepth + bias < objectDepth) {
				percentage -= 1./ 49.;
			}
		}
	}

	return percentage;
}

float HardShadow(vec2 shadowUv, float objectDepth, float bias, uint cascadeLevel) {
	vec2 uv = shadowUv;
	float sampledDepth = texture(shadowMap, vec3(uv, cascadeLevel)).r;
	
	if(sampledDepth + bias < objectDepth) {
		return 0.;
	}
	return 1.;
}

#define EPS 1e-3
#define PI 3.141592653589793
#define PI2 6.283185307179586

#define BLOCKER_SEARCH_NUM_SAMPLES 16 
#define PCF_NUM_SAMPLES 16 
#define NEAR_PLANE 0.1
#define LIGHT_WORLD_SIZE .2
#define LIGHT_FRUSTUM_WIDTH 3.75 
#define LIGHT_SIZE_UV LIGHT_WORLD_SIZE/LIGHT_FRUSTUM_WIDTH

vec2 poissonDisk[16] = vec2[16]( 
 vec2( -0.94201624, -0.39906216 ), 
 vec2( 0.94558609, -0.76890725 ), 
 vec2( -0.094184101, -0.92938870 ), 
 vec2( 0.34495938, 0.29387760 ), 
 vec2( -0.91588581, 0.45771432 ), 
 vec2( -0.81544232, -0.87912464 ), 
 vec2( -0.38277543, 0.27676845 ), 
 vec2( 0.97484398, 0.75648379 ), 
 vec2( 0.44323325, -0.97511554 ), 
 vec2( 0.53742981, -0.47373420 ), 
 vec2( -0.26496911, -0.41893023 ), 
 vec2( 0.79197514, 0.19090188 ), 
 vec2( -0.24188840, 0.99706507 ), 
 vec2( -0.81409955, 0.91437590 ), 
 vec2( 0.19984126, 0.78641367 ), 
 vec2( 0.14383161, -0.14100790 )
 );

float GetDirShadowBias(vec3 normal, vec3 lightDir)
{	
	const float MINIMUM_SHADOW_BIAS = 0.0001;
	float bias = max(MINIMUM_SHADOW_BIAS * (1.0 - dot(normal, lightDir)), MINIMUM_SHADOW_BIAS);
	return MINIMUM_SHADOW_BIAS;
}


float PenumbraSize(float zReceiver, float zBlocker) //Parallel plane estimation
{ 
 return (zReceiver - zBlocker) / zBlocker; 
} 

float SearchRegionRadiusUV(float zWorld)
{
	const float light_zNear = 0.0; // 0.01 gives artifacts? maybe because of ortho proj?
	const float lightRadiusUV = 0.05;
	return lightRadiusUV * (zWorld - light_zNear) / zWorld;
}

vec2 SamplePoisson(int index)
{
	return poissonDisk[index % 16];
}

float FindBlockerDistance(vec3 shadowCoords, float uvLightSize, float bias) {
	int blockers = 0;
	float avgBlockerDistance = 0.;

	float searchWidth =  LIGHT_SIZE_UV * (shadowCoords.z - NEAR_PLANE) / shadowCoords.z;
//	float searchWidth = SearchRegionRadiusUV(shadowCoords.z);

	for (int i = 0; i < 16; i++)
	{
		float z = texture(shadowMap, vec3(shadowCoords.xy + SamplePoisson(i) * searchWidth, 0)).r;
		if (z < (shadowCoords.z - bias))
		{
			blockers++;
			avgBlockerDistance += z;
		}
	}

	if (blockers > 0) {
		return avgBlockerDistance / float(blockers);
	} else {
		return -1;
	}
}

float NV_PCF_DirectionalLight(vec3 shadowCoords, float uvRadius, float bias)
{
	float sum = 0;
	for (int i = 0; i < 16; i++)
	{
		vec2 offset = poissonDisk[i] * uvRadius;
		float z = texture(shadowMap, vec3(shadowCoords.xy + offset, 0)).r;
		sum += step(shadowCoords.z - bias, z);
	}
	return sum / 16.0f;
}

float PCF_DirectionalLight(vec3 shadowCoords, float uvRadius, float bias)
{
	int numPCFSamples = 16;

	float sum = 0;
	for (int i = 0; i < numPCFSamples; i++)
	{
		vec2 offset = SamplePoisson(i) * uvRadius;
		float z = texture(shadowMap, vec3(shadowCoords.xy + offset, 0)).r;
		sum += step(shadowCoords.z - bias, z);
	}
	return sum / numPCFSamples;
}

float PCSS(vec3 shadowCoords, float bias, float uvLightSize) {
	float blockerDistance  = FindBlockerDistance(shadowCoords, uvLightSize, bias);

	if(blockerDistance == -1) {
		return 1.; // no occlusion
	}

	float penumbraWidth = (shadowCoords.z - blockerDistance) / blockerDistance;

	float uvRadius = penumbraWidth * LIGHT_SIZE_UV * NEAR_PLANE / shadowCoords.z;

	return NV_PCF_DirectionalLight(shadowCoords, uvRadius, bias);
}

vec3 CalculatePointLight(PBRParameters params) {

	vec3 color = vec3(0.);

	for(uint i = 0; i < u_PointLights.lightCount; i++) {
//	for(uint i = 0; i < 1; i++) {
		PointLight light = u_PointLights.lights[i];

		vec3 albedo =  params.albedo;
		float metalness = params.metalness;
		float roughness = params.roughness;
		vec3 position = params.position;

		vec3 ray = light.position - position;

//		float lightDistance = length(light.position - position);
//		float lightDistance = 2;

//		float attenuation = 1 / (1 + 2 / light.radius * lightDistance + (lightDistance * lightDistance / light.radius * light.radius));

//		float attenuation = clamp(1.0 - (lightDistance * lightDistance) / (light.radius * light.radius), 0.0, 1.0);
//		attenuation *= mix(attenuation, 1.0, light.falloff);
//
//		float attenuation =  1.0 / (1. + .09 * lightDistance + 
//    		    .032 * (lightDistance * lightDistance));

//		float denom = lightDistance / light.radius + 1.;
//		float attenuation = 1. / (denom * denom);
//		attenuation = (attenuation - light.falloff) / (1. - light.falloff);
//		attenuation = max(attenuation, 0.);
//

		float distanceSqr = max(dot(ray, ray), 0.00001);
		float rangeAttenuation = clamp(1.0 - (distanceSqr * distanceSqr / light.radius/ light.radius/ light.radius/ light.radius), 0., 1.);
		rangeAttenuation *= rangeAttenuation;
		float attenuation = rangeAttenuation;

		vec3 radiance = light.radiance * attenuation;


		// normal
		vec3 n = params.normal;
		// inverse light
		vec3 l = normalize(light.position - position);
		// view/eye vector
		vec3 v = normalize(globalRenderData.eye);
		// half vector of light and view
		vec3 h = normalize(l + v);

		float NoH = max(dot(n, h), 0.);
		float NoV = params.NoV;
		float NoL = max(dot(n, l), 0.);
		float VoH = max(dot(v, h), 0.);

		// conductor doesn't have diffuse effect
		vec3 diffuseColor =  albedo;

		// Fresnel factor at incidence angle differs between dielectric and conductor
		// reflectance of conductor is chromatic, determined by it's albedo(reflectance)
		// reflectance of dielectric is achromatic, generally speaking 0.04 is good enough for most of the material
		vec3 f0 = dielectricReflectance * (1. - metalness) + albedo * metalness;

		// fresnel term for specular
		// this is also used to determine the weight of diffuse brdf and specular brdf
		vec3 f_Schlick = F_Schlick(f0, VoH);

		// cook-torrance microfacet specular brdf: NormalDistribution * GeometryOcclusion * Fresnel
		vec3 f_Specular = D_GGX(roughness, NoH) * V_SmithGGXCorrelated(NoV, NoL, roughness) * f_Schlick;
		// lambertian diffuse brdf
		vec3 f_Diffuse = Fd_Lambertian() * diffuseColor;

		vec3 kd = (1. - f_Schlick) * (1. - metalness);

		color += (kd * f_Diffuse + f_Specular) * radiance * NoL;
	}

	return max(vec3(0.), color);
}


void main() {

//------------------------------------------------------------------------------------------------------
// Lighting

	vec3 albedo = materialUniforms.AlbedoColor * texture(albedoMap, uv).xyz;
	vec3 emission = texture(emissionMap, uv).xyz + materialUniforms.Emission;
	float metalness = materialUniforms.Metalness * texture(metallicRoughnessMap, uv).b;
	float roughness = materialUniforms.Roughness * texture(metallicRoughnessMap, uv).g;

	roughness = clamp(roughness, .05, 1.);
	// normal
	vec3 n = normalize(tbn * normalize( normalize(texture(normalMap, uv) * 2. - 1.).xyz).xyz * materialUniforms.UseNormalMap + (1. - materialUniforms.UseNormalMap) * normal);

	// inverse light
	vec3 l = normalize(-globalRenderData.directionalLightDirection);
	// view/eye vector
	vec3 v = normalize(globalRenderData.eye - viewPos.xyz);
	// half vector of light and view
	vec3 h = normalize(l + v);

    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);

//		float NoH = max(dot(n, h), 0.);
//	float NoV = clamp(dot(n, v) + 1e-5, 0, 1.);
//	float NoL = clamp(dot(n, l) + 1e-5, 0, 1.);
//	float VoH = max(dot(v, h), 0.);


	PBRParameters params;
	params.albedo = albedo;
	params.position = viewPos.xyz;
	params.metalness = metalness;
	params.normal = n;
	params.roughness = roughness;
	params.view = v;
	params.NoV = NoV;
	// conductor doesn't have diffuse effect
	vec3 diffuseColor =  albedo.rgb;

	// Fresnel factor at incidence angle differs between dielectric and conductor
	// reflectance of conductor is chromatic, determined by it's albedo(reflectance)
	// reflectance of dielectric is achromatic, generally speaking 0.04 is good enough for most of the material
	vec3 f0 = dielectricReflectance * (1. - metalness) + albedo * metalness;

	// fresnel term for specular
	// this is also used to determine the weight of diffuse brdf and specular brdf
	vec3 f_Schlick = F_Schlick(f0, LoH);

	// cook-torrance microfacet specular brdf: NormalDistribution * GeometryOcclusion * Fresnel
	vec3 f_Specular = D_GGX(roughness, NoH) * V_SmithGGXCorrelated(NoV, NoL, roughness) * f_Schlick;
	// lambertian diffuse brdf
	vec3 f_Diffuse = Fd_Lambertian() * diffuseColor;

	vec3 kd = (1. - f_Schlick) * (1. - metalness);
// Lighting end
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
// Shadow
	
	uint cascadeLevel = 0;
	float centerDistance = 0.;
	float cascadeRadius = 0.;

	for(uint i = 0; i < 4; i++) {
		vec3 sphereCenter = globalRenderData.cascadeSphereInfo[i].xyz;
		float sphereRadius = globalRenderData.cascadeSphereInfo[i].w;
		float d = length(sphereCenter - viewPos.xyz);
		if(d < sphereRadius) {
			cascadeRadius = sphereRadius;
			centerDistance = d;
			cascadeLevel = i;
			break;
		}
	}

	vec4 lightSpaceCorrected = globalRenderData.lightVPCascade[cascadeLevel] * viewPos;

	vec3 shadowMapUV = lightSpaceCorrected.xyz / lightSpaceCorrected.w;

	vec2 texUV = vec2(shadowMapUV.x * .5 + .5, shadowMapUV.y * .5 + .5);
	
	// apply shadow
	float shadow = 1.;
	float shadowBias = GetDirShadowBias(n, l);

//	shadow = HardShadow(texUV, shadowMapUV.z, shadowBias, cascadeLevel);
	shadow = PCF_Box7x7(texUV, shadowMapUV.z, shadowBias, cascadeLevel);

	// smooth transition between cascades
	if(true) {
		float transition = globalRenderData.cascadeSplits[cascadeLevel + 1].y;

		if(cascadeRadius - centerDistance < transition) {
		
			vec4 a = globalRenderData.lightVPCascade[cascadeLevel + 1] * viewPos;
			float shadowNext = PCF_Box7x7(vec2(a.x * .5 + .5, a.y * .5 + .5), a.z, shadowBias, cascadeLevel + 1);

			shadow = mix(shadowNext, shadow, (cascadeRadius - centerDistance) / transition);
		}
	}
// Shadow End
//---------------------------------------------------------------------------------------------------------------------

	// final shading
	color = vec4(((kd * f_Diffuse + f_Specular) * globalRenderData.directionalLightRadiance * NoL + emission), 1.);

//	color += vec4(CalculatePointLight(params), 1.);
//	if(color.x > 1.) {
//		color = vec4(1., 0., 0., 1.);
//	}
//
//	switch(cascadeLevel) {
//		case 0:
//			color *= vec4(1., 0.25, 0.25, 1.);
//			break;
//		case 1:
//			color *= vec4(0.25, 1., 0.25, 1.);
//			break;
//		case 2:
//			color *= vec4(0.25, 0.25, 1., 1.);
//			break;
//		case 3:
//			color *= vec4(1., .25, .25, 1.);
//			break;
//	}
}