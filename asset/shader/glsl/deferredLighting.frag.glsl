// reference: https://google.github.io/filament/Filament.md.html#materialsystem

#version 450 core

#define M_PI 3.1415926535897932384626433832795
#define MAX_POINT_LIGHT_COUNT 1000
const float Epsilon = 0.00001;
const float ShadowBias = 0.005;

layout(location = 0) in vec2 uv;

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

struct PointLight {
	vec3 position;
	float radius;
	vec3 radiance;
	float falloff;
};

layout(set = 0, binding = 1) uniform sampler2DArray shadowMap;
layout(set = 0, binding = 2) uniform PointLightData {
	PointLight lights[MAX_POINT_LIGHT_COUNT];
	uint lightCount;
} u_PointLights;
layout(set = 0, binding = 3) uniform samplerCube specularMap;
layout(set = 0, binding = 4) uniform sampler2D brdfLut;

layout(set = 1, binding = 0) uniform sampler2D albedoMetalnessMap;
layout(set = 1, binding = 1) uniform sampler2D normalRoughnessMap;
layout(set = 1, binding = 2) uniform sampler2D positionOcclusionMap;
layout(set = 1, binding = 3) uniform sampler2D emissionShadowMap;


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

float V_SmithGGXCorrelated(float NoV, float NoL, float a) {

    float a2 = a * a;
    float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return 0.5 / max((GGXV + GGXL), 1e-5);
}

float Fd_Lambertian() {
	return 1. / M_PI;
}

vec3 FresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float Pow5(float x)
{
    return (x * x * x * x * x);
}


vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * Pow5(1.0 - cosTheta);
}


const vec3 dielectricReflectance = vec3(0.04);

float unpack(vec4 rgbaDepth) {
    const vec4 bitShift = vec4(1.0, 1.0/256.0, 1.0/(256.0*256.0), 1.0/(256.0*256.0*256.0));
    return dot(rgbaDepth, bitShift);
}

// 7*7 box filter
float PCF_Box7x7(vec2 shadowUv, float objectDepth, float bias, uint cascade) {

	vec2 texSize = textureSize(shadowMap, 0).xy;

	vec2 texelSize = 1. / texSize;

	float percentage = 1.;

	for(float i = -3.; i < 3.; i++) {
		for(float j = -3.; j < 3.; j++) {
			vec2 uv = shadowUv + texelSize * vec2(i, j);
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

vec3 CalculatePointLight(PBRParameters params) {

	vec3 color = vec3(0.);
	return color;

	for(uint i = 0; i < u_PointLights.lightCount; i++) {
		PointLight light = u_PointLights.lights[i];

		vec3 albedo =  params.albedo;
		float metalness = params.metalness;
		float roughness = params.roughness;
		vec3 position = params.position;

		vec3 ray = light.position - position;

		float distanceSqr = max(dot(ray, ray), 1e-5);
		float radiusQuad = light.radius * light.radius * light.radius * light.radius;
		float rangeAttenuation = clamp(1.0 - (distanceSqr * distanceSqr / radiusQuad), 0., 1.);
		rangeAttenuation *= rangeAttenuation;

		vec3 radiance = light.radiance * rangeAttenuation;


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
		float NoL = clamp(dot(n, l) + 1e-5, 0, 1.);
		float VoH = max(dot(v, h), 0.);
		float LoH = max(dot(l, h), 0.);

		// conductor doesn't have diffuse effect
		vec3 diffuseColor =  albedo;

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

		color += (kd * f_Diffuse + f_Specular) * radiance * NoL;
	}
	return max(vec3(0.), color);
}

#define EPS 1e-3
#define PI 3.141592653589793
#define PI2 6.283185307179586

float GetDirShadowBias(vec3 normal, vec3 lightDir, float cascade)
{	
	const float MINIMUM_SHADOW_BIAS = 0.001;
	float bias = max(MINIMUM_SHADOW_BIAS * (1.0 - dot(normal, lightDir)), MINIMUM_SHADOW_BIAS) * (cascade + 1.);
	return bias;
}

// TODO: move this to env map
// yokohama
const vec3[9] gSH9Color = vec3[9](
vec3(0.187413, 0.117695, 0.0971929),
vec3(0.0110947, -0.00559425, -0.0095226),
vec3(-0.0176567, -0.00918142, -0.00868316),
vec3(-0.0211396, -0.0150282, -0.0219823),
vec3(0.0161046, 0.0136101, 0.0100705),
vec3(0.00540282, 0.00556897, 0.00673108),
vec3(-0.0206313, -0.00648618, -0.00285285),
vec3(0.0276528, 0.0245642, 0.0239924),
vec3(0.187039, 0.110578, 0.0859501)
);

const float A0 = sqrt(4. * PI);
const float A1 = sqrt(4. * PI / 3.);
const float A2 = sqrt(4. * PI / 5.);

// IBL diffuse SH
vec3 IBLDiffuseSH(vec3 N) {

    vec3 irradiance = gSH9Color[0] * 0.282095f * A0
        + gSH9Color[1] * 0.488603f * N.y * A1
        + gSH9Color[2] * 0.488603f * N.z * A1
        + gSH9Color[3] * 0.488603f * N.x * A1
        + gSH9Color[4] * 1.092548f * N.x * N.y * A2
        + gSH9Color[5] * 1.092548f * N.y * N.z * A2
        + gSH9Color[6] * 0.315392f * (3.0f * N.z * N.z - 1.0f) * A2
        + gSH9Color[7] * 1.092548f * N.x * N.z * A2
        + gSH9Color[8] * 0.546274f * (N.x * N.x - N.y * N.y) * A2;
	return irradiance;
}

float SpecularAntiAliasing(vec3 n, float rs) {
	float SIGMA2 = 0.15915494;
	float KAPPA = 0.18;

	vec3 dndu = dFdx(n);
	vec3 dndv = dFdy(n);

	float kernelRoughness2 = 2. * SIGMA2 * (dot(dndu, dndu) + dot(dndv, dndv));
	float clampedKernelRoughness2 = min(kernelRoughness2, KAPPA);
	float filteredRoughness2 = clamp(rs + clampedKernelRoughness2, 0., 1.);

	return sqrt(filteredRoughness2);

}

void main() {

//------------------------------------------------------------------------------------------------------
// Lighting

	vec4 albedoMetalness = texture(albedoMetalnessMap, uv);
	vec4 normalRoughness = texture(normalRoughnessMap, uv);
	vec4 positionOcclusion = texture(positionOcclusionMap, uv);
	vec4 emissionShadow = texture(emissionShadowMap, uv);
	
	vec3 emission = emissionShadow.rgb;
	float receiveShadow = emissionShadow.a;
	vec3 position = positionOcclusion.rgb;
	vec3 albedo =  albedoMetalness.rgb;
	vec3 n = normalize(normalRoughness.rgb);
	float metalness = albedoMetalness.a;
	float roughness = normalRoughness.a;

	if(length(normalRoughness.rgb) == 0) {
		color = vec4(0., 0., 0., 1.);
		return;
	}
	
	// specular anti aliasing
	roughness = SpecularAntiAliasing(n, roughness * roughness);

	// inverse light
	vec3 l = normalize(-globalRenderData.directionalLightDirection);
	// view/eye vector
	vec3 v = normalize(globalRenderData.eye - position);
	// half vector of light and view
	vec3 h = normalize(l + v);

	float NoH = max(dot(n, h), 0.);
	float NoV = clamp(dot(n, v) + 1e-5, 0, 1.);
	float NoL = clamp(dot(n, l) + 1e-5, 0, 1.);
	float VoH = max(dot(v, h), 0.);
	float LoH = max(dot(l, h), 0.);

	PBRParameters params;
	params.albedo = albedo;
	params.position = position;
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
		float d = length(sphereCenter - position);
		if(d < sphereRadius) {
			cascadeRadius = sphereRadius;
			centerDistance = d;
			cascadeLevel = i;
			break;
		}
	}

	vec4 lightSpaceCorrected = globalRenderData.lightVPCascade[cascadeLevel] * vec4(position, 1.);

	vec3 shadowMapUV = lightSpaceCorrected.xyz / lightSpaceCorrected.w;

	vec2 texUV = vec2(shadowMapUV.x * .5 + .5, shadowMapUV.y * .5 + .5);
	
	// apply shadow
	float shadow = 1.;
	float shadowBias = GetDirShadowBias(n, l, cascadeLevel);

	shadow = PCF_Box7x7(texUV, shadowMapUV.z, shadowBias, cascadeLevel);

	// smooth transition between cascades
	if(true) {
		float transition = globalRenderData.cascadeSplits[cascadeLevel + 1].y;

		if(cascadeRadius - centerDistance < transition) {
			
			float shadowBias1 = GetDirShadowBias(n, l, cascadeLevel + 1);
			vec4 a = globalRenderData.lightVPCascade[cascadeLevel + 1] * vec4(position, 1.);
			float shadowNext = PCF_Box7x7(vec2(a.x * .5 + .5, a.y * .5 + .5), a.z, shadowBias1, cascadeLevel + 1);

			shadow = mix(shadowNext, shadow, (cascadeRadius - centerDistance) / transition);
		}
	}
// Shadow End
//---------------------------------------------------------------------------------------------------------------------
	color = vec4(vec3(0.), 1.);
	// directional light shading
	color = vec4(((kd * f_Diffuse + f_Specular) * globalRenderData.directionalLightRadiance * NoL), 1.);
	if(receiveShadow == 1.) {
		color = vec4((color.rgb * shadow), 1.);
	}
	// point light shading
	vec3 pointLightShading = CalculatePointLight(params);
	color += vec4(pointLightShading, 1.);

	vec3 diffuseIBL = IBLDiffuseSH(n) * albedo;

	float r = sqrt(roughness);

	vec3 R = normalize(reflect(-v, n));
//	R = vec3(R.x, R.z, R.y);
	vec3 reflection = textureLod(specularMap, R, r * 4.).rgb;

	vec3 F        = F_SchlickR(clamp(dot(h, v), 0.0, 1.0), f0, r);
	vec2 brdfLUT = texture(brdfLut, vec2(clamp(dot(h, v), 0.0, 1.0), r)).rg;

	vec3 specularIBL = reflection * (F * brdfLUT.x + brdfLUT.y);

	
	vec3 kD = 1.0 - F;
	kD *= 1.0 - metalness;

	vec3 IBL = (kD * diffuseIBL + specularIBL);

	color += vec4(IBL, 1.);

	// ambient
	color += vec4(albedo * vec3(0.03), 1.);

	// emission
	color += vec4(emission, 1.);

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