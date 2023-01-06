#version 450 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(set = 0, binding = 0, rgba8) writeonly uniform imageCube target;
layout(set = 0, binding = 1) uniform samplerCube source;

layout(push_constant) uniform SMU {
	float roughness;
} smu;

#define PI 3.14159265359

float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
} 

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;
	
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
	
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}  

float D_GGX(float roughness, float NoH) {
	float a = NoH * roughness;
	float k = roughness/(1. - NoH * NoH + roughness * roughness);
	return k * k / PI;
}

// order: px nx py ny pz nz
void main() {
	uvec3 xyz = gl_GlobalInvocationID.xyz;

	vec2 imgSize = imageSize(target);

	vec2 uv = (gl_GlobalInvocationID.xy + .5) / imageSize(target).xy;
	uv = 2. * uv - 1.;

	vec3 n;
	switch(xyz.z)
	{
		case 0:
			n = normalize(vec3(1., -uv.yx));
			break;
		case 1:
			n = normalize(vec3(-1., -uv.y, uv.x));
			break;
		case 2:
			n = normalize(vec3(uv.x, 1, uv.y));
			break;
		case 3:
			n = normalize(vec3(uv.x, -1, -uv.y));
			break;
		case 4:
			n = normalize(vec3(uv.x, -uv.y, 1));
			break;
		case 5:
			n = normalize(vec3(-uv, -1));
			break;
		default:
			break;
	}

	vec3 N = n;
	vec3 V = n;
	vec3 R = n;
	
	const uint SAMPLE_COUNT = 4096u;
	float totalWeight = 0.;
	vec3 prefilteredColor = vec3(0.);

	for(uint i = 0u; i < SAMPLE_COUNT; i++) 
	{
	    vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, smu.roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
		float NoH = clamp(dot(N, H), 0., 1.);
		float HoV = clamp(dot(H, V), 0., 1.);

		float D   = D_GGX(smu.roughness, NoH);
		float pdf = (D * NoH / (4.0 * HoV)) + 0.0001; 

		float resolution = 512.0; // resolution of source cubemap (per face)
		float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
		float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

		float mipLevel = smu.roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 
        if(NdotL > 0.0)
        {
            prefilteredColor += textureLod(source, L, mipLevel).rgb * NdotL;
            totalWeight      += NdotL;
        }
	}
	prefilteredColor /= totalWeight;

	imageStore(target, ivec3(xyz), vec4(prefilteredColor, 1.));
}