#version 460
#pragma sde include "noise3D.glsl"

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;
layout(r16f, binding = 0) uniform image3D theTexture;

uniform vec4 WorldOffset;
uniform vec4 CellSize;

uniform vec4 PlanetCenter;
uniform float PlanetRadius;

float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); 
}

float RidgeNoise(vec3 p) 
{
	return 2.0 * (0.5 - abs(0.5 - snoise(vec3(p.x,p.y,p.z))));
};

float Union(float p0, float p1)
{
	return min(p0, p1);
};

float Subtract(float d1, float d2)
{
	return max(-d1, d2);
};

float SDF(vec3 worldPos)
{
	float d = length(worldPos - PlanetCenter.xyz) - PlanetRadius;
	float caves = RidgeNoise(vec3(worldPos.x * 0.02, worldPos.y * 0.02, worldPos.z * 0.02));
	float innerCore = length(worldPos - vec3(512,512,512)) - 400;
	d = min(d,length(worldPos - vec3(512,512,512)) - PlanetRadius - RidgeNoise(vec3(5 + worldPos.x * 0.01, 3 + worldPos.y * 0.01, 1 + worldPos.z * 0.01)) * 60);
	//d = Subtract(caves, d);
	//d = Union(d, innerCore);

	return d;
}

void main() {
  
  // get index in global work group i.e x,y position
  ivec3 pixel_coords = ivec3(gl_GlobalInvocationID.xyz);
  vec3 worldPos = WorldOffset.xyz + CellSize.xyz * vec3(pixel_coords);
  
  float d = SDF(worldPos);
  
  // output to a specific pixel in the image
  // WHY IS IT CLAMPED?!
  imageStore(theTexture, pixel_coords, vec4(d,0,0,0));
}	