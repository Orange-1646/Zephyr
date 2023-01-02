#version 450 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba16) uniform writeonly image2D result;
layout(set = 0, binding = 1) uniform sampler2D inputImage;

layout(push_constant) uniform DownsampleData
{
	int mip;
} downsampleData;

// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
float LinearRgbToLuminance(vec3 linearRgb) {
	return dot(linearRgb, vec3(0.2126729f,  0.7151522f, 0.0721750f));
}

vec3 Downsample13(sampler2D image, float mip, vec2 uv, vec2 texelSize) {

    vec3 A = textureLod(image, uv, 0).rgb;

    vec3 B = textureLod(image, uv + vec2(-1., 1.) * texelSize, mip).rgb;
    vec3 C = textureLod(image, uv + vec2(-1., -1.) * texelSize, mip).rgb;
    vec3 D = textureLod(image, uv + vec2(1., -1.) * texelSize, mip).rgb;
    vec3 E = textureLod(image, uv + vec2(1., 1.) * texelSize, mip).rgb;
    
    vec3 F = textureLod(image, uv + vec2(-2., 2.) * texelSize, mip).rgb;
    vec3 G = textureLod(image, uv + vec2(0., 2.) * texelSize, mip).rgb;
    vec3 H = textureLod(image, uv + vec2(2., 2.) * texelSize, mip).rgb;
    vec3 I = textureLod(image, uv + vec2(2., 0.) * texelSize, mip).rgb;
    vec3 J = textureLod(image, uv + vec2(2., -2.) * texelSize, mip).rgb;
    vec3 K = textureLod(image, uv + vec2(0., -2.) * texelSize, mip).rgb;
    vec3 L = textureLod(image, uv + vec2(-2., -2.) * texelSize, mip).rgb;
    vec3 M = textureLod(image, uv + vec2(-2., 0.) * texelSize, mip).rgb;

    vec3 a = (B + C + D + E) * .5;
    vec3 b = (F + G + A + M) * .125;
    vec3 c = (G + H + I + A) * .125;
    vec3 d = (A + I + J + K) * .125;
    vec3 e = (M + A + K + L) * .125;
    // partial karis average on first iteration of downsample to prevent firefly artifacts
    if(downsampleData.mip == 0) {
        float lumaWeightA =  1 / (1 + LinearRgbToLuminance(A));
        float lumaWeightB =  1 / (1 + LinearRgbToLuminance(B));
        float lumaWeightC =  1 / (1 + LinearRgbToLuminance(C));
        float lumaWeightD =  1 / (1 + LinearRgbToLuminance(D));
        float lumaWeightE =  1 / (1 + LinearRgbToLuminance(E));
        float lumaWeightF =  1 / (1 + LinearRgbToLuminance(F));
        float lumaWeightG =  1 / (1 + LinearRgbToLuminance(G));
        float lumaWeightH =  1 / (1 + LinearRgbToLuminance(H));
        float lumaWeightI =  1 / (1 + LinearRgbToLuminance(I));
        float lumaWeightJ =  1 / (1 + LinearRgbToLuminance(J));
        float lumaWeightK =  1 / (1 + LinearRgbToLuminance(K));
        float lumaWeightL =  1 / (1 + LinearRgbToLuminance(L));
        float lumaWeightM =  1 / (1 + LinearRgbToLuminance(M));

        float totalWeightA = lumaWeightB + lumaWeightC + lumaWeightD + lumaWeightE;
        float totalWeightB = lumaWeightF + lumaWeightG + lumaWeightA + lumaWeightM;
        float totalWeightC = lumaWeightG + lumaWeightH + lumaWeightI + lumaWeightA;
        float totalWeightD = lumaWeightA + lumaWeightI + lumaWeightJ + lumaWeightK;
        float totalWeightE = lumaWeightM + lumaWeightA + lumaWeightK + lumaWeightL;

        a = (B * lumaWeightB + C * lumaWeightC + D * lumaWeightD + E * lumaWeightE)/totalWeightA * .5;
        b = (F * lumaWeightF + G * lumaWeightG + A * lumaWeightA + M * lumaWeightM)/totalWeightB * .5;
        c = (G * lumaWeightG + H * lumaWeightH + I * lumaWeightI + A * lumaWeightA)/totalWeightC * .5;
        d = (A * lumaWeightA + I * lumaWeightI + J * lumaWeightJ + K * lumaWeightK)/totalWeightD * .5;
        e = (M * lumaWeightM + A * lumaWeightA + K * lumaWeightK + L * lumaWeightL)/totalWeightE * .5;
    }

    return (a + b + c + d + e) * .25;
}

void main() {

    vec2 imgSize = imageSize(result).xy;
    
    vec2 texSize = vec2(textureSize(inputImage, 0));
    


    ivec2 invocID = ivec2(gl_GlobalInvocationID);
    if(invocID.x > imgSize.x || invocID.y > imgSize.y) {
        return;
    }
    vec2 texCoords = vec2(float(invocID.x) / imgSize.x, float(invocID.y) / imgSize.y);
    texCoords += (1.0f / imgSize) * 0.5f;
    vec2 texelSize = 1./texSize;

    vec3 color = Downsample13(inputImage, 0, texCoords, texelSize);

    imageStore(result, invocID, vec4(color, 1.));
}