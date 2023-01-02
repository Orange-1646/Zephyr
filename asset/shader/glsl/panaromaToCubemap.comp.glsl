#version 450 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(set = 0, binding = 0, rgba16) writeonly uniform  imageCube resultCubemap;
layout(set = 0, binding = 1) uniform sampler2D equirectangular;

void main() {

}