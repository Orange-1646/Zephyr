#version 450 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba8) uniform writeonly image2D result;
layout(set = 0, binding = 1) uniform sampler2D inputTexture;

layout(push_constant, std140) uniform FXAAData {
	float contrastThreshold;
	float relativeThreshold;
	float sharpness;
} fxaaData;

#define EDGE_STEP_COUNT 10
#define EDGE_STEPS 1, 1.5, 2, 2, 2, 2, 2, 2, 2, 4
#define EDGE_GUESS 8

const float edgeSteps[EDGE_STEP_COUNT] = {EDGE_STEPS};

/*
	NW	N	NE

	W	M	E

	SW	S	SE
*/

struct LumaData {
	float m, n, s, w, e;
	float nw, ne, sw, se;
	float highest, lowest, contrast;
};

struct EdgeData {
	bool isHorizontal;
	float pixelStep;
	float oppositeLuma, gradient;
};

bool ShouldSkipPixel(LumaData l) {
	float threshold = max(fxaaData.contrastThreshold, fxaaData.relativeThreshold * l.highest);

	return l.contrast < threshold;
}

float SampleLuma(vec2 uv) {
	return textureLod(inputTexture, uv, 0).a;
}

LumaData SampleNeighborLuma(vec2 uv, vec2 texelSize) {
	LumaData data;
	data.m = SampleLuma(uv);
	data.n = SampleLuma(uv + vec2(0., texelSize.y));
	data.s = SampleLuma(uv + vec2(0., -texelSize.y));
	data.e = SampleLuma(uv + vec2(texelSize.x, 0.));
	data.w = SampleLuma(uv + vec2(-texelSize.x, 0.));
	data.nw = SampleLuma(uv + vec2(-texelSize.x, texelSize.y));
	data.ne = SampleLuma(uv + vec2(texelSize.x, texelSize.y));
	data.sw = SampleLuma(uv + vec2(-texelSize.x, -texelSize.y));
	data.se = SampleLuma(uv + vec2(texelSize.x, -texelSize.y));
	
	data.highest = max(data.m, max(data.n, max(data.s, max(data.w, data.e))));
	data.lowest = min(data.m, min(data.n, min(data.s, min(data.w, data.e))));
	data.contrast = data.highest - data.lowest;

	return data;
}

/* 3x3 tent filter
	1	2	1
	2		1
	1	2	1
*/

// this determines the subpixel blending factor
float DeterminePixelBlendFactor(LumaData data) {
	float factor = 0.;
	factor += data.nw + data.ne + data.sw + data.se;
	factor += 2. * (data.n + data.s + data.w + data.e);

	factor /= 12.;
	factor = abs(data.m - factor);
	factor = clamp(factor/data.contrast, 0., 1.);
	factor = smoothstep(0., 1., factor);
	factor *= factor * fxaaData.sharpness;

	return factor;
}

// this determines the blending along the edge
float DetermineEdgeBlendFactor(LumaData l, EdgeData e, vec2 uv, vec2 texelSize) {
	vec2 edgeUv = uv;
	vec2 edgeStep;
	if(e.isHorizontal) {
		edgeUv.y += e.pixelStep * .5;
		edgeStep = vec2(texelSize.x, 0.);
	} else {
		edgeUv.x += e.pixelStep * .5;
		edgeStep = vec2(0., texelSize.y);
	}

	// walk along both direction to find two ends of the edge
	float edgeLuma = (l.m + e.oppositeLuma) * .5;
	float gradientThreshold = e.gradient * .25;

	vec2 pUv = edgeUv;
	float pLumaDelta;
	float nLumaDelta;
	vec2 nUv = edgeUv;

	bool pAtEnd = false;
	bool nAtEnd = false;

	for(int i = 0; i < EDGE_STEP_COUNT && !pAtEnd; i++) {
		pUv += edgeStep * edgeSteps[i];
		pLumaDelta = SampleLuma(pUv) - edgeLuma;
		pAtEnd = abs(pLumaDelta) >= gradientThreshold;
	}
	if(!pAtEnd) {
		pUv += edgeStep * EDGE_GUESS;
	}

	for(int i = 0; i < EDGE_STEP_COUNT && !nAtEnd; i++) {
		nUv -= edgeStep * edgeSteps[i];
		nLumaDelta = SampleLuma(nUv) - edgeLuma;
		nAtEnd = abs(nLumaDelta) >= gradientThreshold;
	}
	if(!nAtEnd) {
		nUv -= edgeStep * EDGE_GUESS;
	}

	float pDistance;
	float nDistance;
		
	if(e.isHorizontal) {
		pDistance = pUv.x - edgeUv.x;
		nDistance = edgeUv.x - nUv.x;
	} else {
		pDistance = pUv.y - edgeUv.y;
		nDistance = edgeUv.y - nUv.y;
	}

	float shortestDistance;
	bool deltaSign;

	if(pDistance < nDistance) {
		shortestDistance = pDistance;
		deltaSign = pLumaDelta >= 0.;
	} else {
		shortestDistance = nDistance;
		deltaSign = nLumaDelta >= 0.;
	}

	if(deltaSign == (l.m - e.oppositeLuma) >= 0.) {
		return 0.;
	} else {
		return .50001 - shortestDistance / (pDistance + nDistance);
	}

}

// do a comparison between horizontal contrast and vertical contrast.
// we use different weight for diagonal values
EdgeData DeterminEdge(LumaData data, vec2 texelSize) {
	EdgeData e;

	float horizontal = abs(data.n + data.s - 2. * data.m) * 2. + abs(data.ne + data.se - 2. * data.e) + abs(data.nw + data.sw - 2. * data.w);

	float vertical = abs(data.w + data.e - 2. * data.m) * 2. + abs(data.nw + data.ne - 2. * data.n) + abs(data.sw + data.se - 2. * data.s);

	// determin which direction to blend. positive direction: n for horizontal and e for vertical

	e.isHorizontal = horizontal > vertical;
	e.pixelStep = e.isHorizontal ? texelSize.y : texelSize.x;
	
	float pLuma = e.isHorizontal ? data.n : data.e;
	float nLuma = e.isHorizontal ? data.s : data.w;
	float pGradient = abs(data.m - pLuma);
	float nGradient = abs(data.m - nLuma);

	if(pGradient < nGradient) {
		e.pixelStep = -e.pixelStep;
		e.oppositeLuma = nLuma;
		e.gradient = nGradient;
	} else {
		e.oppositeLuma = pLuma;
		e.gradient = pGradient;
	}


	return e;
}

vec3 Sample(vec2 uv) {
	return textureLod(inputTexture, uv, 0).rgb;
}

vec3 ApplyFXAA(vec2 uv, vec2 texelSize) {
	LumaData data = SampleNeighborLuma(uv, texelSize);

	if(ShouldSkipPixel(data)) {
		return Sample(uv);
	}
	float pixelBlendFactor = DeterminePixelBlendFactor(data);

	EdgeData e = DeterminEdge(data, texelSize);
	
	float edgeBlendFactor = DetermineEdgeBlendFactor(data, e, uv, texelSize);

	float finalBlendFactor = max(edgeBlendFactor, pixelBlendFactor);

	if(e.isHorizontal) {
		uv.y += e.pixelStep * finalBlendFactor;
	} else {
		uv.x += e.pixelStep * finalBlendFactor;
	}


	return Sample(uv);
}

// we do fxaa in linear space(before gamma correction)
void main() {
	vec2 texSize = imageSize(result);
	vec2 uv = (vec2(gl_GlobalInvocationID) + .5) / texSize;

	vec2 texelSize = 1./texSize;

	imageStore(result, ivec2(gl_GlobalInvocationID.xy), vec4(ApplyFXAA(uv, texelSize), SampleLuma(uv)));
}