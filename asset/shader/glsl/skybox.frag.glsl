#version 450 core

layout(location = 0) in vec3 inDirection;

layout(location = 0) out vec4 color;

layout(set = 0, binding = 1) uniform samplerCube skybox;

#define PI 3.141592653589793
// yokohama
//const vec3[9] gSH9Color = vec3[9](
//vec3(0.187413, 0.117695, 0.0971929),
//vec3(0.0110947, -0.00559425, -0.0095226),
//vec3(-0.0176567, -0.00918142, -0.00868316),
//vec3(-0.0211396, -0.0150282, -0.0219823),
//vec3(0.0161046, 0.0136101, 0.0100705),
//vec3(0.00540282, 0.00556897, 0.00673108),
//vec3(-0.0206313, -0.00648618, -0.00285285),
//vec3(0.0276528, 0.0245642, 0.0239924),
//vec3(0.187039, 0.110578, 0.0859501)
//);

// grace cathedral
//const vec3[9] gSH9Color = vec3[9](
//vec3(1.39351, 0.784853, 0.752926),
//vec3(0.129026, 0.149647, 0.162697),
//vec3(0.0197399, 0.00963427, 0.0712005),
//vec3(-0.0339507, -0.00624006, -0.0840478),
//vec3(0.134226, 0.113336, 0.176077),
//vec3(0.00318684, 0.0711683, 0.0416948),
//vec3(-0.24864, -0.213819, -0.210963),
//vec3(-0.211575, -0.124532, -0.0635663),
//vec3(0.233533, 0.133927, -0.00847369)
//);

// indoor
const vec3[9] gSH9Color = vec3[9](
vec3(0.51847, 0.510833, 0.498099),
vec3(-0.0139187, -0.0198636, -0.0233149),
vec3(-0.0229853, -0.0361403, -0.0237941),
vec3(0.0263349, 0.0681706, 0.0585434),
vec3(-0.0508704, -0.0607166, -0.0570893),
vec3(0.0514967, 0.0357191, 0.0207576),
vec3(0.0147249, 0.0112061, -0.0267427),
vec3(0.00411591, 0.0257378, 0.042852),
vec3(0.0642033, 0.0399831, 0.0190294)
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

void main() {
	color = texture(skybox, inDirection);
}