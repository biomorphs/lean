#version 430

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(r8, binding = 0) uniform image3D theTexture;

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
  
  float d = length(worldPos - vec3(32,37,32)) - 4 - (10 * (1.0 + sin(Time * 0.5)));
  d = opSmoothUnion(d,worldPos.y - 32 - sin(worldPos.x * 0.5 + Time) * 2 - cos(worldPos.z * 0.25 + Time),1.0);
  
  // output to a specific pixel in the image
  imageStore(theTexture, pixel_coords, vec4(clamp(d,-1,1),0,0,0));
}	