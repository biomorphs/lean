#version 430 
#pragma sde include "shared.fs"

in vec4 out_colour;
in vec2 out_uv;
out vec4 colour;
uniform sampler2D DiffuseTexture;
 
void main()
{
	vec4 finalColour = srgbToLinear(texture(DiffuseTexture, out_uv)) * out_colour;
	vec3 toneMapped = Tonemap_ACESFilm(finalColour.rgb * HDRExposure);
	colour = vec4(linearToSRGB(toneMapped),finalColour.a);
}