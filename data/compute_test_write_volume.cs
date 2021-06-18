#version 430
#pragma sde include "noise2D.glsl"

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;
layout(r16f, binding = 0) uniform image3D OutVolume;

uniform vec4 WorldOffset;
uniform vec4 CellSize;

uniform float Time;

float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); 
}

float RidgeNoise(vec2 p) 
{
	return 0;
	//return 2.0 * (0.5 - abs(0.5 - snoise(vec3(p.x,0,p.y))));
};

float Plane(vec3 p, vec3 n, float h)
{
	return dot(p, n) + h;
};

float Union(float p0, float p1)
{
	return min(p0, p1);
};

float Subtract(float d1, float d2)
{
	return max(-d1, d2);
};

float SDTest(vec3 worldPos)
{
	float d = length(worldPos - vec3(1500,200,1500)) - 180;

	float noises = snoise(vec2(Time + worldPos.x * 0.00025,worldPos.z * 0.00025)) * 1;
	noises = noises + snoise(vec2(Time + worldPos.x * 0.005,worldPos.z * 0.005)) * 0.5;
	noises = noises + snoise(vec2(Time + worldPos.x * 0.01,worldPos.z * 0.01)) * 0.25;
	noises = noises + snoise(vec2(Time + worldPos.x * 0.02,worldPos.z * 0.02)) * 0.125;
	noises = noises + snoise(vec2(Time + worldPos.x * 0.04,worldPos.z * 0.04)) * 0.03125;
	noises = noises + snoise(vec2(Time + worldPos.x * 0.8,worldPos.z * 0.8)) * 0.015625;
	noises = noises + snoise(vec2(Time + worldPos.x * 0.16,worldPos.z * 0.16)) * 0.0078125;
	d = min(d,worldPos.y - 40 - noises * 400);
	d = opSmoothUnion(d,worldPos.y - 4 - sin(worldPos.x * 0.13 + Time) * 2 - cos(worldPos.z * 0.05 + Time),1.0);
	return d;
}

float SDFTestFromCpp(float x, float y, float z)
{
	float terrainNoise = RidgeNoise(vec2(12.3f + x * 0.01f, Time + 51.2f + z * 0.01f)) * 1.0f +
						 RidgeNoise(vec2(40.1f + x * 0.02f, Time + 27.2f + z * 0.02f)) * 0.5f + 
						 RidgeNoise(vec2(97.4f + x * 0.04f, Time + 64.2f + z * 0.04f)) * 0.25f + 
						 RidgeNoise(vec2(13.1f + x * 0.08f, Time + 89.2f + z * 0.08f)) * 0.125f + 
						 RidgeNoise(vec2(76.3f + x * 0.16f, Time + 12.2f + z * 0.16f)) * 0.0625f;
	terrainNoise = terrainNoise / (1.0f + 0.5f +	0.25f +	0.125f + 0.0625f);
	float d = y - 16 - terrainNoise * 128;
	//d = Subtract(snoise(vec3(x * 0.008f,y * 0.008f, z * 0.008f) * 4.0f), d);
	//d = Union(d,Plane(vec3( x,y,z ), vec3(0.0f,1.0f,0.0f), 0.0f));
	return d;
}

void main() {
  
	// get index in global work group i.e x,y position
	ivec3 p = ivec3(gl_GlobalInvocationID.xyz);
	vec3 worldPos = WorldOffset.xyz + CellSize.xyz * vec3(p);

	float d = SDTest(worldPos);

	imageStore(OutVolume, p, vec4(d,0,0,0));
}	