#version 430 
#pragma sde include "shared.fs"

uniform sampler2D SourceTexture;

in vec2 vs_out_position;
out vec4 fs_out_colour;
 
void main()
{
	vec2 uv = (vs_out_position + 1.0) * 0.5;
    vec3 finalColour = vec3(texture(SourceTexture, uv));
	finalColour = Tonemap_ACESFilm(finalColour.rgb);
	fs_out_colour = vec4(linearToSRGB(finalColour),1.0);
}