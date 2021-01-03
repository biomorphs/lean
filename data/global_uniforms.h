// Global uniforms

struct LightInfo
{
	vec4 ColourAndAmbient;
	vec4 Position;
	vec3 Attenuation;
	vec3 ShadowParams;		// enabled, far plane
};

layout(std140, binding = 0) uniform Globals
{
	mat4 ProjectionViewMatrix;
	vec4 CameraPosition;	// World Space
	LightInfo Lights[64];
	int LightCount;
	float HDRExposure;
	float ShadowBias;
	float CubeShadowBias;
};

uniform mat4 ShadowLightSpaceMatrix;
