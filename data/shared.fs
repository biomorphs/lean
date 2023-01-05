#pragma sde include "per_instance_data.h"
#pragma sde include "global_uniforms.h"

// utility functions
const float c_gamma = 2.2;

vec4 srgbToLinear(vec4 v)
{
	return vec4(pow(v.rgb, vec3(c_gamma)), v.a);
}

vec3 linearToSRGB(vec3 v)
{
	return pow(v.rgb, vec3(1.0 / c_gamma));
}

// from https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 Tonemap_ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e),0.0,1.0);
}

ivec2 GetScreenTileIndices(vec2 glFragCoord)
{
	vec2 tileDimensions = WindowDimensions / LightTileCounts;
	return ivec2(glFragCoord / tileDimensions);
}

uint GetLightTileIndex(ivec2 screenTileIndices)
{
	return screenTileIndices.x + (screenTileIndices.y * LightTileCounts.x);
}