#version 430 
#pragma sde include "shared.fs"

in vec3 vs_out_position;

uniform int ShadowLightIndex;

void main()
{
	if(Lights[ShadowLightIndex].Position.w == 0.0 || Lights[ShadowLightIndex].Position.w == 2.0)
	{
		gl_FragDepth = gl_FragCoord.z;
	}
	else
	{
		// get distance between fragment and light source
		float lightDistance = length(vs_out_position.xyz - Lights[ShadowLightIndex].Position.xyz);

		// map to [0;1] range by dividing by far_plane
		lightDistance = lightDistance / Lights[ShadowLightIndex].DistanceAttenuation.x;
    
		// write this as modified depth
		gl_FragDepth = lightDistance;
	}
}