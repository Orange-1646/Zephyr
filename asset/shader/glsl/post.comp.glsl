#version 450 core

layout(local_size_x = 16, local_size_y = 16) in;
//layout(local_size_x = 256) in;

layout(set = 0, binding = 0, rgba8) uniform image2D result;
layout(set = 0, binding = 1) uniform sampler2D inputImage;

float conv(in float[9] kernel, in float[9] data, in float denom, in float offset) 
{
   float res = 0.0;
   for (int i=0; i<9; ++i) 
   {
      res += kernel[i] * data[i];
   }
   return clamp(res/denom + offset, 0.0, 1.0);
}

struct ImageData 
{
	float avg[9];
} imageData;	

void main()
{	
	vec2 rtSize = imageSize(result);
	// Fetch neighbouring texels
	int n = -1;
	for (int i=-1; i<2; ++i) 
	{   
		for(int j=-1; j<2; ++j) 
		{    
			n++;    
			vec2 uv = vec2(float(gl_GlobalInvocationID.x + i)/rtSize.x, float(gl_GlobalInvocationID.y + j)/rtSize.y) + .5 * 1./rtSize;
			vec3 rgb = texture(inputImage, uv).rgb;
			imageData.avg[n] = (rgb.r + rgb.g + rgb.b) / 3.0;
		}
	}

	float[9] kernel;
	kernel[0] = -1.0/8.0; kernel[1] = -1.0/8.0; kernel[2] = -1.0/8.0;
	kernel[3] = -1.0/8.0; kernel[4] =  1.0;     kernel[5] = -1.0/8.0;
	kernel[6] = -1.0/8.0; kernel[7] = -1.0/8.0; kernel[8] = -1.0/8.0;
									
	vec4 res = vec4(vec3(conv(kernel, imageData.avg, 0.1, 0.0)), 1.0);

//	imageStore(result, ivec2(gl_GlobalInvocationID.xy), res);
	vec2 uv = vec2(float(gl_GlobalInvocationID.x)/rtSize.x, float(gl_GlobalInvocationID.y)/rtSize.y);
	uv += 1./imageSize(result) * .5;

	imageStore(result, ivec2(gl_GlobalInvocationID.xy), vec4(texture(inputImage, uv).rgb, 1.));
}