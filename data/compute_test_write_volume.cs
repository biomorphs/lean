#version 430
#pragma sde include "noise2D.glsl"

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(r16f, binding = 0) uniform image3D theTexture;

uniform vec4 WorldOffset;
uniform vec4 CellSize;

uniform float Time;

float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); 
}

void main() {
  
  // get index in global work group i.e x,y position
  ivec3 pixel_coords = ivec3(gl_GlobalInvocationID.xyz);
  vec3 worldPos = WorldOffset.xyz + CellSize.xyz * vec3(pixel_coords);
  
  float d = length(worldPos - vec3(10,32,-32)) - 6 - (16 * (1.0 + sin(Time * 0.5)));
  d = opSmoothUnion(d,worldPos.y - 4 - sin(worldPos.x * 0.13 + Time) * 2 - cos(worldPos.z * 0.05 + Time),1.0);
  
  float noises = snoise(vec2(Time + worldPos.x * 0.005,worldPos.z * 0.005)) * 1;
  noises = noises + snoise(vec2(Time + worldPos.x * 0.01,worldPos.z * 0.01)) * 0.5;
  noises = noises + snoise(vec2(Time + worldPos.x * 0.02,worldPos.z * 0.02)) * 0.25;
  noises = noises + snoise(vec2(Time + worldPos.x * 0.04,worldPos.z * 0.04)) * 0.125;
  noises = noises + snoise(vec2(Time + worldPos.x * 0.08,worldPos.z * 0.08)) * 0.03125;
  noises = noises + snoise(vec2(Time + worldPos.x * 0.16,worldPos.z * 0.16)) * 0.015625;
  noises = noises + snoise(vec2(Time + worldPos.x * 0.32,worldPos.z * 0.32)) * 0.0078125;
  d = opSmoothUnion(d,worldPos.y - 20 - noises * 50,2);
  
  // output to a specific pixel in the image
  imageStore(theTexture, pixel_coords, vec4(clamp(d,-1,1),0,0,0));
}	