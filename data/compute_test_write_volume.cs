#pragma sde include "noise2D.glsl"
uniform float Time;

float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); 
}

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

float SDF(vec3 worldPos)
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