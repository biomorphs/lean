// Global uniforms

struct LightInfo
{
	vec4 ColourAndAmbient;
	vec4 Position;			// w = type (0=directional,1=point,2=spot)
	vec3 Direction;
	vec2 DistanceAttenuation;
	vec2 SpotlightAngles;	// inner, outer (cosines for faster math)
	vec3 ShadowParams;		// enabled, shadowmap index, bias
	mat4 LightspaceTransform;
};

layout(std140, binding = 0) uniform Globals
{
	mat4 ProjectionViewMatrix;
	vec4 CameraPosition;	// World Space
	LightInfo Lights[64];
	int LightCount;
	float HDRExposure;
};
