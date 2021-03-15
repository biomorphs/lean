#version 430 
#pragma sde include "shared.fs"

uniform sampler2D SourceTexture;
uniform sampler2D LightingTexture;

in vec2 vs_out_position;
out vec4 fs_out_colour;
 
void main()
{
	vec2 uv = (vs_out_position + 1.0) * 0.5;
    fs_out_colour = texture(SourceTexture, uv) + texture(LightingTexture, uv);
}