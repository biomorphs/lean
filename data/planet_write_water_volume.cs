#version 460

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;
layout(r16f, binding = 0) uniform image3D theTexture;
uniform vec4 WorldOffset;
uniform vec4 CellSize;

uniform vec4 PlanetCenter;
uniform float OceanRadius;

float SDF(vec3 worldPos)
{
	float d = length(worldPos - PlanetCenter.xyz) - OceanRadius;
	return d;
}

void main() {
  
  // get index in global work group i.e x,y position
  ivec3 pixel_coords = ivec3(gl_GlobalInvocationID.xyz);
  vec3 worldPos = WorldOffset.xyz + CellSize.xyz * vec3(pixel_coords);
  
  float d = SDF(worldPos);
  
  imageStore(theTexture, pixel_coords, vec4(d,0,0,0));
}	