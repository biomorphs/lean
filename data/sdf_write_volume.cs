#version 460

#pragma sde include "SDF_SHADER_INCLUDE"

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;
layout(r16f, binding = 0) uniform image3D DistanceField;
uniform vec4 WorldOffset;
uniform vec4 CellSize;

void main() {
  ivec3 p = ivec3(gl_GlobalInvocationID.xyz);
  vec3 worldPos = WorldOffset.xyz + CellSize.xyz * vec3(p);
  
  float d = SDF(worldPos);
  
  imageStore(DistanceField, p, vec4(d,0,0,0));
}	