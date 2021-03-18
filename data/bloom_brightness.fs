#version 430 
#pragma sde include "shared.fs"

uniform sampler2D SourceTexture;
uniform float BrightnessThreshold;
uniform float BrightnessMulti;

in vec2 vs_out_position;

out vec4 fs_out_colour;
 
void main()
{
	vec2 uv = (vs_out_position + 1.0) * 0.5;
	vec4 colour = texture(SourceTexture, uv);
	float brightness = dot(colour.rgb, vec3(0.2126, 0.7152, 0.0722));
	if(brightness > BrightnessThreshold)
	{
		fs_out_colour = colour * clamp((brightness - BrightnessThreshold) * BrightnessMulti,0.0,1.0);
	}
	else
	{
		fs_out_colour = vec4(0.0,0.0,0.0,1.0);
	}
}