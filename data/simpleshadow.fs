#version 430 
#pragma sde include "shared.fs"

in vec3 vs_out_position;
in vec2 vs_out_uv;

uniform int ShadowLightIndex;
uniform sampler2D DiffuseTexture;
uniform vec4 MeshDiffuseOpacity;

void main()
{
	vec4 diffuseTex = texture(DiffuseTexture, vs_out_uv);	
	if(diffuseTex.a < 0.5 || MeshDiffuseOpacity.a == 0.0)
		discard;

	if(Lights[ShadowLightIndex].Position.w == 0.0)
	{
		gl_FragDepth = gl_FragCoord.z;
	}
	else
	{
		// get distance between fragment and light source
		float lightDistance = length(vs_out_position.xyz - Lights[ShadowLightIndex].Position.xyz);

		// map to [0;1] range by dividing by far_plane
		lightDistance = lightDistance / Lights[ShadowLightIndex].ShadowParams.y;
    
		// write this as modified depth
		gl_FragDepth = lightDistance;
	}
}