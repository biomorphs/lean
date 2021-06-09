#version 430 
#pragma sde include "shared.fs"

in vec3 vs_out_position;
out vec4 fs_out_colour;
 
void main()
{
	vec3 finalColour = vec3(1.0);
	
	// apply exposure here, assuming next pass is postfx
	fs_out_colour = vec4(finalColour.rgb * HDRExposure,1.0);
}