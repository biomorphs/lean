#version 430 
#pragma sde include "shared.fs"

uniform sampler3D SourceTexture;
uniform float Slice = 0;

in vec2 vs_out_position;
out vec4 fs_out_colour;
 
void main()
{
	vec2 uv = (vs_out_position + 1.0) * 0.5;
	fs_out_colour = texture(SourceTexture, vec3(uv,Slice));
}