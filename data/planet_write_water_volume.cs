uniform vec4 PlanetCenter;
uniform float OceanRadius;

float SDF(vec3 worldPos)
{
	float d = length(worldPos - PlanetCenter.xyz) - OceanRadius;
	return d;
}