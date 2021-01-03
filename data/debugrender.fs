#version 430 
#pragma sde include "shared.fs"

in vec4 out_colour;
out vec4 colour;
 
void main()
{
	vec3 toneMapped = Tonemap_ACESFilm(out_colour.rgb * HDRExposure);
	colour = vec4(linearToSRGB(toneMapped),out_colour.a);
}