#version 430 
#pragma sde include "shared.fs"

in vec4 out_colour;
out vec4 colour;
 
void main()
{
	colour = vec4(out_colour.rgb * HDRExposure,out_colour.a);
}