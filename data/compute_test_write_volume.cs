#version 430

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(r8, binding = 0) uniform image3D theTexture;

uniform float Time;

void main() {
  
  // get index in global work group i.e x,y position
  ivec3 pixel_coords = ivec3(gl_GlobalInvocationID.xyz);
  
  float d = length(vec3(pixel_coords) - vec3(64,64,64)) - 32;
  
  // output to a specific pixel in the image
  imageStore(theTexture, pixel_coords, vec4(d,0,0,0));
}